import voxie
import voxie.dbus_as_json


def save_properties(data, json_data, child_name='DataProperties'):
    if child_name in json_data:
        raise Exception('json_data dict already has an entry for {!r}'.format(child_name))

    data = voxie.castImplicit(data, 'de.uni_stuttgart.Voxie.Data')

    props = data.ListProperties()
    if len(props) == 0:
        return

    prop_data = {}
    for prop in props:
        name = prop.Name
        if name in prop_data:
            raise Exception('name in prop_data')
        sig = prop.Type.DBusSignature
        value = data.GetProperty(prop).getValue(sig)
        value_json = voxie.dbus_as_json.encode_dbus_as_json(sig, value)
        val = prop.PropertyDefinition
        val['Value'] = value_json
        prop_data[name] = val
        # print(val)

    json_data[child_name] = prop_data


# TODO: Get instance somehow from data object?
def load_properties(instance, data, update, json_data, child_name='DataProperties'):
    data = voxie.castImplicit(data, 'de.uni_stuttgart.Voxie.Data')

    if child_name not in json_data:
        return
    props = json_data[child_name]

    for prop_name in props:
        prop_json = dict(props[prop_name])

        value_json = prop_json['Value']
        del prop_json['Value']

        prop = instance.CreateDataProperty(prop_name, prop_json)

        sig = prop.Type.DBusSignature
        value = voxie.dbus_as_json.decode_dbus_as_json(sig, value_json)

        data.SetProperty(update, prop, voxie.Variant(sig, value), {
            'ReplaceMode': voxie.Variant('s', 'de.uni_stuttgart.Voxie.ReplaceMode.Insert'),
        })
