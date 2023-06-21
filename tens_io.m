function h = tens_io(device_id)

cmds.identify = 'i';
cmds.get_clock = 't';
cmds.begin_sync = 's';
cmds.check_sync = 'S';
cmds.tens_on = 'n';
cmds.tens_off = 'f';
cmds.tens_status = 'p';

port_ptr = IOPort('OpenSerialPort', device_id, ...
    'BaudRate=115200 Lenient');

IOPort('Write', device_id, uint8(cmds.identify));
id_str = '';
endl = sprintf('\n');
while len(id_str) == 0 || id_str(end) != endl
    data = IOPort('Read', device_id);
    id_str = [id_str, char(data(:)')];
end
h = struct;
h.id_str = id_str;
h.power_on = @()power_on(device_id);
h.power_off = @()power_off(device_id);
h.check_status = @()check_status(device_id);
h.begin_sync = @()begin_sync(device_id);


function t = power_on(device_id)
    [~, t] = IOPort('Write', device_id, uint8(cmds.tens_on));
end

function t = power_off(device_id)
    [~, t] = IOPort('Write', device_id, uint8(cmds.tens_off));
end

function status = check_status(device_id)
    IOPort('Write', device_id, uint8(cmds.tens_status));
    status = IOPort('Read', device_id, blocking=1, 1);
end

function checker = begin_sync(device_id)
    [~, write_ts] = IOPort('Write', device_id, uint8(cmds.get_clock));
    data = IOPort('Read', device_id, blocking=1, 4);
    device_ts = typecast(data, 'uint32');
    time_delta = write_ts - double(device_ts)/1000;
    IOPort('Write', device_id, cmds.begin_sync);
    checker = @()check_sync(device_id, time_delta);
end

function sync_ts = check_sync(device_id, time_delta)
    IOPort('Write', device_id, uint8(cmds.check_sync));
    data = IOPort('Read', device_id, blocking=1, 4);
    device_ts = typecast(data, 'uint32');
    if device_ts > 0
        sync_ts = double(device_ts)/1000 + time_delta;
    else
        sync_ts = 0;
    end
end
