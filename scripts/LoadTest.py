#!/usr/bin/python3

import numpy
import voxie
import dbus
import time
import sys

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

if args.voxie_action is not None and args.voxie_action != 'Load':
    raise Exception('Invalid operation: ' + args.voxie_action)

with instance.createClient() as client:
    with instance.claimExternalOperation(client, args.voxie_operation) as op:
        origin = (4.0, 5.0, 6.0)
        spacing = (0.1, 0.1, 0.1)

        with instance.createVoxelData(client, (10, 20, 40), {'Origin': origin, 'Spacing': spacing}) as data, data.getDataWritable() as buffer:
            True
            #time.sleep(1)
            raise Exception('foo')
            op.interface('Load').Finish(data.path)
