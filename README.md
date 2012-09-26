
# erl_i2c
 ------------------------------------------------------------

Erlang I2C C-Node and gen_server frontend
Developed for and tested on a RaspberryPi running ArchLinux and Erlang R15B02

* erl has to be started with an valid shortname (-sname)  
* a secret-cookie must be set either via a '`.erlang.cookie`'-file or via cmd-line option to erl (-setcookie)  

## Start gen_server and C-Node

The C-Node, responsible for execution of required ioctls, is started with the  
start of the gen_server:

* `erl_i2c:start_link()`

## Connect to i2c-bus

* `erl_i2c:open_bus(BusNum)`  
opens a i2c-bus with number `BusNum` - this corresponds to the device `/dev/i2c-BusNum`  
returns `{open_bus, ok, BusNum}` on success and  
`{open_bus, error, Reason}` on error  
(as long as the corresponding devices exist, multiple i2c-buses could be opened)

* `erl_i2c:close_bus(BusNum)`  
closes a previously opened i2c-bus with number `BusNum`  
returns `{close_bus, ok}` on success and   
`{close_bus, error, Reason}` on error  

## Write bytes on i2c- bus
To send data to devices connected to an i2c-bus the `erl_i2c:write_byte`-function is exported.  
It comes in different flavours / arities:

(where '`Bus_Number`', '`Device_Address`' and '`Device_Register`' are of type `erlang::integer`  
and '`Device_Data`' is of type `erlang::binary` and must not exceed a length of 32 bytes)  

* `write_byte(Bus_Number, Device_Address, Device_Register, Device_Data)`
* `write_byte(Device_Address, Device_Register, Device_Data)`
* `write_byte(Device_Register, Device_Data)`
* `write_byte(Device_Data)`

returns `{write_byte, ok, Bytes_Written}` on success and  
`{write_byte, error, Reason}` on error

'`Bus_Number`', '`Device_Address`' and '`Device_Register`' are remembered on consecutive writes.  
As long as you're sure there's no other erlang-process accessing the bus you're using you could do:  
`write_byte(0, 32, 1, <<0>>),`  
`write_byte(<<1>>),`  
`write_byte(<<2>>).`  
which is the same as `write_byte(0,32, 1, <<0, 1, 2>>).`
 
## Read Bytes from i2c-bus
To read data from devices connected to an i2c-bus the `erl_i2c:read_byte`-function is exported.  
It comes in different flavours/arities:

(where '`Bus_Number`', '`Device_Address`', '`Data_Length`' and '`Read_Data_Length`' are of type `erlang::integer`  
'`Data_Length`' must not exceed 32  
'`Read_Data`' is of type `erlang::binary`)  

* `read_byte(Bus_Number, Device_Address, Device_Register, Data_Length)`
* `read_byte(Device_Address, Device_Register, Data_Length)`
* `read_byte(Device_Register, Data_Length)`
* `read_byte(Data_Length)`

returns `{read_byte, ok, Read_Data_Length, Read_Data}` on success and  
`{read_byte, error, Reason}` on error

'`Bus_Number`', '`Device_Address`' and '`Device_Register`' are remembered on consecutive reads.

## Other Functions - mentioned but currently not documented
* `erl_i2c:bus_info/0,1`
* `erl_i2c:set_address/1,2`
* `erl_i2c:get_address/0,1`
* `erl_i2c:set_bus/1`
* `erl_i2c:get_bus/0`

### Author
Christian Adams <morlac78@googlemail.com>

### License
GPLv2

### Disclaimer
This documentation and the documented C-Node and gen_server is far from beeing complete.  


