import abc
import os

import numpy as np

import voxie

# TODO: Test whether the StorageOrder stuff work properly (for both vol and raw)

# TODO: Check checksum in iai file?

# TODO: Merge some code for DenseBinaryFile and ImageArchive


def throwOnUnexpectedMember(data, desc, expected):
    expected = set(expected)
    optional = set()
    if 'OptionalMembers' in data:
        optional = set(data['OptionalMembers'])
    for key in data:
        if key not in expected and key not in optional:
            raise Exception('Unexpected member {!r} in {}'.format(key, desc))


class DataSource(abc.ABC):
    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        return False

    def with_storage_order_output(self, outData):
        # Apply storageOrder
        mirror = []
        dim = [None, None, None]
        for i in range(3):
            if self.storageOrder[i] < 0:
                mirror.append(slice(None, None, -1))
            else:
                mirror.append(slice(None, None))
            d = abs(self.storageOrder[i]) - 1
            if dim[d] is not None:
                raise Exception('dim[d] is not None')
            dim[d] = i
        outData = outData[tuple(mirror)].transpose(dim)
        if outData.shape != tuple(self.diskSize):
            raise Exception('outData.shape != tuple(self.diskSize)')
        return outData

    def get_image_size_disk_coord(self, z):
        val = self.diskSize[:2]
        if val[0] is None or val[1] is None:
            raise Exception('Unable to get image size')
        return tuple(val)

    # Note: Requires self.storageOrder[2] == 3
    def get_image_size(self, z):
        if self.storageOrder[2] != 3:
            raise Exception('Unsupported storage order')

        val = self.get_image_size_disk_coord(z)
        if abs(self.storageOrder[0]) == 1 and abs(self.storageOrder[1]) == 2:
            return val
        elif abs(self.storageOrder[0]) == 2 and abs(self.storageOrder[1]) == 1:
            return (val[1], val[0])
        else:
            raise Exception('Invalid storage order')

    # Note: Requires self.storageOrder[2] == 3
    def read_image(self, z):
        if self.storageOrder[2] != 3:
            raise Exception('Unsupported storage order')

        img = self.read_image_disk_coord(z)

        # TODO: Clean up
        if abs(self.storageOrder[0]) == 1 and abs(self.storageOrder[1]) == 2:
            pass
        elif abs(self.storageOrder[0]) == 2 and abs(self.storageOrder[1]) == 1:
            img = img.transpose()
        else:
            raise Exception('Invalid storage order')
        # TODO: Check this: Should this be here or above the transpose code?
        if self.storageOrder[0] < 0:
            img = img[::-1, :]
        if self.storageOrder[1] < 0:
            img = img[:, ::-1]

        return img


class NotInitializedType:
    pass


NotInitialized = NotInitializedType()


class DataSourceDenseBinaryFileBuffers:
    pass


class DataSourceDenseBinaryFile(DataSource):
    def __init__(self, filename, data, *, array_shape, data_type):
        throwOnUnexpectedMember(data, 'JSON object "DataSource"', ('DataFilename', 'Offset', 'StorageOrder', 'ValueOffset', 'ValueScalingFactor'))

        for dim in array_shape:
            if dim is None:
                raise Exception('DenseBinaryFile requires ArrayShape/ImageShape values')

        dataFilename = str(data['DataFilename'])
        self.offset = int(data.get('Offset', 0))
        if self.offset < 0:
            raise Exception('Invalid file offset')
        if 'StorageOrder' in data:
            self.storageOrder = [int(data['StorageOrder'][i]) for i in range(3)]
        else:
            self.storageOrder = [1, 2, 3]

        self.valueOffset = data.get('ValueOffset')
        self.valueScalingFactor = data.get('ValueScalingFactor')

        self.rawDataFilename = os.path.join(os.path.dirname(filename), dataFilename)

        self.diskSize = [NotInitialized, NotInitialized, NotInitialized]
        for resDim in range(3):
            so = self.storageOrder[resDim]
            aso = abs(so)
            if aso not in [1, 2, 3]:
                raise Exception('Got invalid StorageOrder value: %s' % (so,))
            if self.diskSize[aso - 1] is not NotInitialized:
                raise Exception('Got StorageOrder value %s multiple time' % (aso,))
            self.diskSize[aso - 1] = array_shape[resDim]

        fileType = data_type
        if fileType[1] % 8 != 0:
            raise Exception('Data type size is not multiple of 8')

        # TODO: Create wrapper function in voxie namespace for this
        self.fileDtype = voxie.buffer.endianMap[fileType[2]] + voxie.buffer.typeNameMap[fileType[0]] + str(fileType[1] // 8)

        self.overallSize = fileType[1] // 8
        self.strides = []
        for size in self.diskSize:
            self.strides.append(self.overallSize)
            self.overallSize *= size

        self.memType = data_type
        if (self.valueOffset is not None or self.valueScalingFactor is not None) and fileType[0] != 'float':
            self.memType = ('float', 32, 'native')
        # Always set self.memType to native endian
        self.memType = (self.memType[0], self.memType[1], 'native')

        # TODO: Create wrapper function in voxie namespace for this
        self.memDtype = voxie.buffer.endianMap[self.memType[2]] + voxie.buffer.typeNameMap[self.memType[0]] + str(self.memType[1] // 8)

        # Various buffer
        # Note: Currently this breaks multithreaded access to a DataSource object
        self.buff = DataSourceDenseBinaryFileBuffers()

        # The data as read from the disk
        # Note: Is transposed (y, x)
        self.buff.imgFile = np.zeros((self.diskSize[1], self.diskSize[0]), dtype=self.fileDtype)
        self.expectedCount = self.strides[2]  # Bytes in imgFile
        # The data, possibly converted to FP and to native endian
        # Note: Is transposed (y, x)
        self.buff.imgConverted = self.buff.imgFile
        if self.valueOffset is not None or self.valueScalingFactor is not None:
            self.buff.imgConverted = np.zeros((self.diskSize[1], self.diskSize[0]), dtype=self.memDtype)
        self.buff.imgConvertedT = self.buff.imgConverted.T

        # Open the actual file
        success = False
        self.file = open(self.rawDataFilename, 'rb')
        try:
            size = self.file.seek(0, 2)
            # print(size, self.diskSize, self.offset, self.overallSize, self.strides)
            if self.offset + self.overallSize > size:
                raise Exception('File {!r} is too small, got {} bytes, but expected at least {} bytes ({} + {})'.format(self.rawDataFilename, size, self.offset + self.overallSize, self.offset, self.overallSize))
            self.file.seek(self.offset, 0)
            success = True
        finally:
            if not success:
                self.file.close()

    def __exit__(self, type, value, traceback):
        self.file.close()
        return False

    def read_image_disk_coord(self, z):
        self.file.seek(self.offset + self.strides[2] * z, 0)

        count = self.file.readinto(self.buff.imgFile)
        if count != self.expectedCount:
            raise Exception('Unexpected EOF')
        if self.valueOffset is not None or self.valueScalingFactor is not None:
            # TODO: Avoid copy if file is already float? Probably not really needed
            self.buff.imgConverted[:, :] = imgFile
            if self.valueScalingFactor is not None:
                self.buff.imgConverted *= self.valueScalingFactor
            if self.valueOffset is not None:
                self.buff.imgConverted += self.valueOffset

        return self.buff.imgConvertedT


class DataSourceImageArchive(DataSource):
    def __init__(self, filename, data, *, array_shape, data_type):
        throwOnUnexpectedMember(data, 'JSON object "DataSource"', ('DataFilename', 'StorageOrder', 'ValueOffset', 'ValueScalingFactor'))

        # Copied from ctscripts
        import imagearchive

        dataFilename = str(data['DataFilename'])
        if 'StorageOrder' in data:
            self.storageOrder = [int(data['StorageOrder'][i]) for i in range(3)]
        else:
            self.storageOrder = [1, 2, 3]

        self.valueOffset = data.get('ValueOffset')
        self.valueScalingFactor = data.get('ValueScalingFactor')

        rawDataFilename = os.path.join(os.path.dirname(filename), dataFilename)
        # iaiFilename = rawDataFilename + '.iai'

        # print (data_type)
        # print (array_shape)
        # print (gridSpacing)
        # print (volumeOrigin)
        # print (self.storageOrder)
        # print (rawDataFilename)
        # # print (iaiFilename)

        self.diskSize = [NotInitialized, NotInitialized, NotInitialized]
        for resDim in range(3):
            so = self.storageOrder[resDim]
            aso = abs(so)
            if aso not in [1, 2, 3]:
                raise Exception('Got invalid StorageOrder value: %s' % (so,))
            if self.diskSize[aso - 1] is not NotInitialized:
                raise Exception('Got StorageOrder value %s multiple time' % (aso,))
            self.diskSize[aso - 1] = array_shape[resDim]

        self.ia = imagearchive.ImageArchive(rawDataFilename)

        if self.diskSize[2] is None:
            self.diskSize[2] = self.ia.count
        if self.ia.count != self.diskSize[2]:
            raise Exception('self.ia.count != self.diskSize[2]')
        for z in range(self.diskSize[2]):
            info = self.ia.info[z]
            if data_type is not None:
                if info['TypeName'] != data_type[0]:
                    raise Exception("info['TypeName'] != data_type[0]")
                if int(info['TypeSize']) != data_type[1]:
                    raise Exception("int(info['TypeSize']) (%d) != data_type[1] (%d)" % (int(info['TypeSize']), data_type[1]))
                if info['TypeEndian'] != data_type[2]:
                    raise Exception("info['TypeEndian'] (%s) != data_type[2] (%s)", (repr(info['TypeEndian']), repr(data_type[2])))
            if self.diskSize[0] is not None and int(info['Width']) != self.diskSize[0]:
                raise Exception("self.diskSize[0] is not None and int(info['Width']) != self.diskSize[0]")
            if self.diskSize[1] is not None and int(info['Height']) != self.diskSize[1]:
                raise Exception("self.diskSize[1] is not None and int(info['Height']) != self.diskSize[1]")

        if data_type is not None:
            self.memType = data_type
            if (self.valueOffset is not None or self.valueScalingFactor is not None) and data_type[0] != 'float':
                self.memType = ('float', 32, 'native')
            # Always set self.memType to native endian
            self.memType = (self.memType[0], self.memType[1], 'native')

            # TODO: Create wrapper function in voxie namespace for this
            self.memDtype = voxie.buffer.endianMap[self.memType[2]] + voxie.buffer.typeNameMap[self.memType[0]] + str(self.memType[1] // 8)
        else:
            self.memType = None
            self.memDtype = None

    def read_image_disk_coord(self, z):
        # TODO: Avoid memory allocations here?
        img = self.ia.readImage(z)
        if self.valueOffset is not None or self.valueScalingFactor is not None:
            # TODO: Avoid memory allocations here
            # TODO: Clean up
            if img.dtype.kind != 'f':
                img = np.array(img, dtype=np.float32)
            else:
                # Make sure img is writable (self.ia.readImage() might return non-writable array)
                img = np.array(img)
            if self.valueScalingFactor is not None:
                img *= self.valueScalingFactor
            if self.valueOffset is not None:
                img += self.valueOffset
        return img

    def get_image_size_disk_coord(self, z):
        return (self.ia.info[z]['Width'], self.ia.info[z]['Height'])


# TODO: Use multi-threading for this class? Can be useful if the zip file uses compression and/or the image file is compressed.
class DataSourceImageList(DataSource):
    def __init__(self, filename, data, *, array_shape, data_type, additional_members):
        throwOnUnexpectedMember(data, 'JSON object "DataSource"', ('StorageOrder', 'ValueOffset', 'ValueScalingFactor', 'ImageFilePattern', 'ImageFileNames', 'FirstImageIndex') + additional_members)

        global PIL
        import PIL.Image

        # Allow opening large images
        PIL.Image.MAX_IMAGE_PIXELS = None

        if 'StorageOrder' in data:
            self.storageOrder = [int(data['StorageOrder'][i]) for i in range(3)]
        else:
            self.storageOrder = [1, 2, 3]

        self.first_image_index = int(data['FirstImageIndex'])

        if 'ImageFilePattern' in data:
            self.image_file_pattern = str(data['ImageFilePattern'])
        else:
            self.image_file_pattern = None
        if 'ImageFileNames' in data:
            explicit_image_file_names = data['ImageFileNames']
            if type(explicit_image_file_names) != dict:
                raise Exception('type(explicit_image_file_names) != dict')
            self.explicit_image_file_names = {}
            for key_str in explicit_image_file_names:
                key = int(key_str)
                if str(key) != key_str:
                    raise Exception('str(key) != key_str')
                if key < 0:
                    raise Exception('key < 0')

                value = str(explicit_image_file_names[key_str])

                if key in self.explicit_image_file_names:
                    raise Exception('key in self.explicit_image_file_names')
                self.explicit_image_file_names[key] = value
        else:
            self.explicit_image_file_names = None
        if self.image_file_pattern is None and self.explicit_image_file_names is None:
            raise Exception('Neither ImageFilePattern nor ImageFileNames is given')

        self.valueOffset = data.get('ValueOffset')
        self.valueScalingFactor = data.get('ValueScalingFactor')

        # print (data_type)
        # print (array_shape)
        # print (gridSpacing)
        # print (volumeOrigin)
        # print (self.storageOrder)

        self.diskSize = [NotInitialized, NotInitialized, NotInitialized]
        for resDim in range(3):
            so = self.storageOrder[resDim]
            aso = abs(so)
            if aso not in [1, 2, 3]:
                raise Exception('Got invalid StorageOrder value: %s' % (so,))
            if self.diskSize[aso - 1] is not NotInitialized:
                raise Exception('Got StorageOrder value %s multiple time' % (aso,))
            self.diskSize[aso - 1] = array_shape[resDim]

        if self.diskSize[2] is None:
            raise Exception('ZippedImages and ImageStack files required the number of images')

        # TODO: Allow this? (Would require a new get_image_size_disk_coord() implementation to get the image size, this probably would mean decompressing the image just to get the size)
        for dim in array_shape:
            if dim is None:
                raise Exception('ZippedImages and ImageStack files require ArrayShape/ImageShape values')

        if data_type is not None:
            # TODO: Create wrapper function in voxie namespace for this
            self.fileDtype = voxie.buffer.endianMap[data_type[2]] + voxie.buffer.typeNameMap[data_type[0]] + str(data_type[1] // 8)

            self.memType = data_type
            if (self.valueOffset is not None or self.valueScalingFactor is not None) and data_type[0] != 'float':
                self.memType = ('float', 32, 'native')
            # Always set self.memType to native endian
            self.memType = (self.memType[0], self.memType[1], 'native')

            # TODO: Create wrapper function in voxie namespace for this
            self.memDtype = voxie.buffer.endianMap[self.memType[2]] + voxie.buffer.typeNameMap[self.memType[0]] + str(self.memType[1] // 8)
        else:
            self.fileDtype = None
            self.memType = None
            self.memDtype = None

        self.filenames = {}
        for z in range(self.diskSize[2]):
            i = self.first_image_index + z
            if self.explicit_image_file_names is not None and i in self.explicit_image_file_names:
                fn = self.explicit_image_file_names[i]
            elif self.image_file_pattern is not None:
                fn = self.image_file_pattern.format(i)
            else:
                raise Exception('Index {} is not in ImageFileNames but ImageFilePattern is not given'.format(i))
            self.filenames[z] = fn

    def read_image_disk_coord(self, z):
        # TODO: Avoid memory allocations here?
        image_filename = self.filenames[z]
        try:
            with self.open_image_file(image_filename) as file:
                img = np.array(PIL.Image.open(file))
        except Exception as e:
            raise Exception('Error while reading image {} from {!r}: {}'.format(z, image_filename, e)) from e

        # Sanity checks
        if self.diskSize[0] is not None and img.shape[0] != self.diskSize[0]:
            raise Exception('self.diskSize[0] is not None and img.shape[0] != self.diskSize[0]')
        if self.diskSize[1] is not None and img.shape[1] != self.diskSize[1]:
            raise Exception('self.diskSize[1] is not None and img.shape[1] != self.diskSize[1]')
        if self.fileDtype is not None and img.dtype != self.fileDtype:
            raise Exception('self.fileDtype is not None and img.dtype ({!r}) != self.fileDtype ({!r})'.format(img.dtype, self.fileDtype))

        if self.valueOffset is not None or self.valueScalingFactor is not None:
            # TODO: Avoid memory allocations here
            # TODO: Clean up
            if img.dtype.kind != 'f':
                img = np.array(img, dtype=np.float32)
            if self.valueScalingFactor is not None:
                img *= self.valueScalingFactor
            if self.valueOffset is not None:
                img += self.valueOffset
        return img


class DataSourceZippedImages(DataSourceImageList):
    def __init__(self, filename, data, *, array_shape, data_type):
        super().__init__(filename, data, array_shape=array_shape, data_type=data_type, additional_members=('DataFilename',))

        global zipfile
        import zipfile

        dataFilename = str(data['DataFilename'])
        rawDataFilename = os.path.join(os.path.dirname(filename), dataFilename)

        # Open the actual file
        success = False
        self.file = zipfile.ZipFile(rawDataFilename, 'r')
        try:
            self.all_filenames = set(self.file.namelist())
            # print(self.all_filenames)

            for fn in self.filenames.values():
                if fn not in self.all_filenames:
                    raise Exception('Cannot find file {!r} in zip file {!r}'.format(fn, rawDataFilename))

            success = True
        finally:
            if not success:
                self.file.close()

    def __exit__(self, type, value, traceback):
        self.file.close()
        return False

    def open_image_file(self, image_filename):
        return self.file.open(image_filename, 'r')


class DataSourceImageStack(DataSourceImageList):
    def __init__(self, filename, data, *, array_shape, data_type):
        super().__init__(filename, data, array_shape=array_shape, data_type=data_type, additional_members=())

        self.filename = filename

    def open_image_file(self, image_filename):
        image_filename = os.path.join(os.path.dirname(self.filename), image_filename)
        return open(image_filename, 'rb')


def get(filename, data_source_type, data_source, *, array_shape, data_type):
    if type(data_source_type) != str:
        raise Exception('type(data_source_type) != str')
    if type(data_source) != dict:
        raise Exception('type(data_source) != dict')

    if data_source_type == 'DenseBinaryFile':
        return DataSourceDenseBinaryFile(filename, data_source, array_shape=array_shape, data_type=data_type)
    elif data_source_type == 'ImageArchive':
        return DataSourceImageArchive(filename, data_source, array_shape=array_shape, data_type=data_type)
    elif data_source_type == 'ZippedImages':
        return DataSourceZippedImages(filename, data_source, array_shape=array_shape, data_type=data_type)
    elif data_source_type == 'ImageStack':
        return DataSourceImageStack(filename, data_source, array_shape=array_shape, data_type=data_type)
    else:
        raise Exception('Unknown DataSourceType: %s' % (repr(data_source_type)))
