%%% -------------------------------------------------------------------
%%% @author : adams
%%% @copyright  : 2011 by Christian Adams <morlac78@googlemail.com> 
%%% @doc :
%%% erl_i2c interfaces i2c-systems via an Erlang C-Node (erl_i2c_cnode). 
%%% Created : 12.09.2012
%%% @end
%%% -------------------------------------------------------------------
-module(erl_i2c).

-author("morlac78@googlemail.com").
-created("Date: 12.09.2012").
-vsn(0.1).

-define(SERVER, ?MODULE).
-define(APP, erl_i2c).

-behaviour(gen_server).
%% --------------------------------------------------------------------
%% Include files
%% --------------------------------------------------------------------

%% --------------------------------------------------------------------
%% External exports
-export([spawn_cnode/0, cnode_started/2,
				 open_bus/1, close_bus/1, get_bus/0, set_bus/1,
				 bus_info/1, bus_info/0, get_state/0,
				 set_address/2, set_address/1,
				 get_address/1, get_address/0,
				 write_byte/4, write_byte/3, write_byte/2, write_byte/1,
				 read_byte/3, read_byte/2, read_byte/1, read_byte/0,
				 start_link/0, stop_link/0]).

%% gen_server callbacks
-export([init/1,
				 handle_call/3, handle_cast/2, handle_info/2,
				 terminate/2, code_change/3]).

-record(state,
				{cnode_port,
				 cnode_nodename}).

%% ====================================================================
%% External functions
%% ====================================================================

start_link() ->
	error_logger:info_report("~p starting", [?MODULE]),

	gen_server:start_link({local, ?MODULE}, ?MODULE, [], []),

	spawn(
		?SERVER,
		spawn_cnode,
		[]).

%% @doc
%% .
%% @end
stop_link() ->
	gen_server:cast(?SERVER, stop).

%% @doc
%% .
%% @end
get_state() ->
	gen_server:call(
		?SERVER,
		{get_state}).

%% @doc
%% .
%% @end
open_bus(Bus_Number) ->
	gen_server:call(
		?SERVER,
		{open_bus, Bus_Number}).

%% @doc
%% .
%% @end
close_bus(Bus_Number) ->
	gen_server:call(
		?SERVER,
		{close_bus, Bus_Number}).

%% @doc
%% .
%% @end
get_bus() ->
	gen_server:call(
		?SERVER,
		{get_bus}).

%% @doc
%% .
%% @end
set_bus(Bus_Number) ->
	gen_server:call(
		?SERVER,
		{set_bus, Bus_Number}).

%% @doc
%% .
%% @end
bus_info(Bus_Number) ->
	gen_server:call(
		?SERVER,
		{bus_info, Bus_Number}).

%% @doc
%% .
%% @end
bus_info() ->
	gen_server:call(
		?SERVER,
		{bus_info}).

%% @doc
%% .
%% @end
set_address(Bus_Number, Device_Address) ->
	gen_server:call(
		?SERVER,
		{set_address, Bus_Number, Device_Address}).

%% @doc
%% .
%% @end
set_address(Device_Address) ->
	gen_server:call(
		?SERVER,
		{set_address, Device_Address}).

%% @doc
%% .
%% @end
get_address(Bus_Number) ->
	gen_server:call(
		?SERVER,
		{get_address, Bus_Number}).

%% @doc
%% .
%% @end
get_address() ->
	gen_server:call(
		?SERVER,
		{get_address}).

%% @doc
%% .
%% @end
write_byte(Bus_Number, Device_Address, Device_Register, Device_Data) when
	is_binary(Device_Data) andalso byte_size(Device_Data) =< 32 ->
	gen_server:call(
		?SERVER,
		{write_byte, Bus_Number, Device_Address, Device_Register, Device_Data}).

%% @doc
%% .
%% @end
write_byte(Device_Address, Device_Register, Device_Data) when
	is_binary(Device_Data) andalso byte_size(Device_Data) =< 32 ->
	gen_server:call(
		?SERVER,
		{write_byte, Device_Address, Device_Register, Device_Data}).

%% @doc
%% .
%% @end
write_byte(Device_Register, Device_Data) when
	is_binary(Device_Data) andalso byte_size(Device_Data) =< 32 ->
	gen_server:call(
		?SERVER,
		{write_byte, Device_Register, Device_Data}).

%% @doc
%% .
%% @end
write_byte(Device_Data) when
	is_binary(Device_Data) andalso byte_size(Device_Data) =< 32 ->
	gen_server:call(
		?SERVER,
		{write_byte, Device_Data}).

%% @doc
%% .
%% @end
read_byte(Bus_Number, Device_Address, Device_Register)  ->
	gen_server:call(
		?SERVER,
		{read_byte, Bus_Number, Device_Address, Device_Register}).

%% @doc
%% .
%% @end
read_byte(Device_Address, Device_Register) ->
	gen_server:call(
		?SERVER,
		{read_byte, Device_Address, Device_Register}).

%% @doc
%% .
%% @end
read_byte(Device_Register) ->
	gen_server:call(
		?SERVER,
		{read_byte, Device_Register}).

%% @doc
%% .
%% @end
read_byte() ->
	gen_server:call(
		?SERVER,
		{read_byte}).

%% @doc
%% .
%% @end
cnode_started(Erlang_Port, NodeName) ->
	gen_server:cast(
		?SERVER,
		{cnode_started, Erlang_Port, NodeName}).

%% ====================================================================
%% Server functions
%% ====================================================================

%% --------------------------------------------------------------------
%% Function: init/1
%% Description: Initiates the server
%% Returns: {ok, State}          |
%%          {ok, State, Timeout} |
%%          ignore               |
%%          {stop, Reason}
%% --------------------------------------------------------------------
init([]) ->
	%% to make this gen_server monitorable by a supervisor
%%	process_flag(trap_exit, true),

	{ok, #state{}}.

%% --------------------------------------------------------------------
%% Function: handle_call/3
%% Description: Handling call messages
%% Returns: {reply, Reply, State}          |
%%          {reply, Reply, State, Timeout} |
%%          {noreply, State}               |
%%          {noreply, State, Timeout}      |
%%          {stop, Reason, Reply, State}   | (terminate/2 is called)
%%          {stop, Reason, State}            (terminate/2 is called)
%% --------------------------------------------------------------------

%% @doc
%% .
%% @end
handle_call({open_bus, Bus_Number}, _From, State) ->
	send_cnode(State#state.cnode_nodename, {open_bus, Bus_Number}),

	Reply = receive_cnode_response(),

	{reply, Reply, State};

%% @doc
%% .
%% @end
handle_call({close_bus, Bus_Number}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{close_bus, Bus_Number}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({get_bus}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{get_bus}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({set_bus, Bus_Number}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{set_bus, Bus_Number}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({bus_info, Bus_Number}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{bus_info, Bus_Number}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({bus_info}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{bus_info}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({set_address, Bus_Number, Device_Address}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{set_address, Bus_Number, Device_Address}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({set_address, Device_Address}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{set_address, Device_Address}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({get_address, Bus_Number}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{get_address, Bus_Number}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({get_address}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{get_address}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({write_byte, Bus_Number, Device_Address, Device_Register, Device_Data}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{write_byte, Bus_Number, Device_Address, Device_Register, Device_Data}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({write_byte, Device_Address, Device_Register, Device_Data}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{write_byte, Device_Address, Device_Register, Device_Data}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({write_byte, Device_Register, Device_Data}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{write_byte, Device_Register, Device_Data}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({write_byte, Device_Data}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{write_byte, Device_Data}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({read_byte, Bus_Number, Device_Address, Device_Register}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{read_byte, Bus_Number, Device_Address, Device_Register}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({read_byte, Device_Address, Device_Register}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{read_byte, Device_Address, Device_Register}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({read_byte, Device_Register}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{read_byte, Device_Register}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({read_byte}, _From, State) ->
	send_cnode(
		State#state.cnode_nodename,
		{read_byte}),

	{reply, receive_cnode_response(), State};

%% @doc
%% .
%% @end
handle_call({get_state}, _From, State) ->
	error_logger:info_msg("handle_call: {get_state}~n"),
	{reply, State, State};

%% @doc
%% to ensure no garbage-messages fill up the gen-server queue
%% unknown requests will be fetched and reportet.
%% @end
handle_call(Request, _From, State) ->
	error_logger:info_msg(
			"handle_call got unknown request:~n~p~n", [Request]),

	{reply, unknown_request, State}.

%% --------------------------------------------------------------------
%% Function: handle_cast/2
%% Description: Handling cast messages
%% Returns: {noreply, State}          |
%%          {noreply, State, Timeout} |
%%          {stop, Reason, State}            (terminate/2 is called)
%% --------------------------------------------------------------------

%% @doc
%% .
%% @end
handle_cast(stop, State) ->
		{stop, normal, State};

%% @doc
%% .
%% @end
handle_cast({cnode_started, Erlang_Port, Nodename}, State) ->
	{noreply,
	 State#state{cnode_nodename = Nodename,
							 cnode_port = Erlang_Port}};

%% @doc
%% .
%% @end
handle_cast(Msg, State) ->
	error_logger:warning_report(
		"got cast of unknown Message:~p~n~p~n", [Msg, State]),
	{noreply, State}.

%% --------------------------------------------------------------------
%% Function: handle_info/2
%% Description: Handling all non call/cast messages
%% Returns: {noreply, State}          |
%%          {noreply, State, Timeout} |
%%          {stop, Reason, State}            (terminate/2 is called)
%% --------------------------------------------------------------------
handle_info(_Info, State) ->
    {noreply, State}.

%% --------------------------------------------------------------------
%% Function: terminate/2
%% Description: Shutdown the server
%% Returns: any (ignored by gen_server)
%% --------------------------------------------------------------------
terminate(Reason, State) ->
	error_logger:info_msg(
		"~p terminating~nReason: ~p~n", [?SERVER, Reason]),
	
	send_cnode(State#state.cnode_nodename, {exit}),
		
	ok.

%% --------------------------------------------------------------------
%% Func: code_change/3
%% Purpose: Convert process state when code is changed
%% Returns: {ok, NewState}
%% --------------------------------------------------------------------
code_change(_OldVsn, State, _Extra) ->
    {ok, State}.

%% --------------------------------------------------------------------
%%% Internal functions
%% --------------------------------------------------------------------

-spec spawn_cnode() -> ok.
%% @doc
%% .
%% @end
spawn_cnode() ->
	Erlang_Port =
		open_port(
			{spawn_executable,
			 filename:join(
				 [code:priv_dir(?APP),"cbin", "erl_i2c_cnode"])},
			[{args, [erlang:get_cookie()]},
			 stream,
			 use_stdio,
			 stderr_to_stdout,
			 {line, 80},
			 exit_status
			]),

	receive_spawned_cnode(Erlang_Port).

-spec receive_spawned_cnode(
				Erlang_Port::port()) -> ok.
%% @doc
%% .
%% @end
receive_spawned_cnode(Erlang_Port) ->
	receive
		{Erlang_Port, {data, {eol, "this.nodename: " ++ NodeName}}} ->
			cnode_started(Erlang_Port, list_to_atom(NodeName)),

			receive_spawned_cnode(Erlang_Port);

		{Erlang_Port, {data, {eol, Line}}} ->
			error_logger:info_msg("Line: ~p~n", [Line]),

			receive_spawned_cnode(Erlang_Port);

		{Erlang_Port, {exit_status, Status}} ->
			case Status of
				0 -> ok;
				_ ->
					error_logger:error_report(
						"got exit_status: ~p~n", [Status])
			end;

		{erl_i2c_cnode, Msg} ->
			error_logger:info_msg(
				"receive_cnode~ngot Message from erl_i2c_cnode:~n~p~n", [Msg]),

			receive_spawned_cnode(Erlang_Port);

		Message ->
			error_logger:info_msg(
				"unknown message: ~p~n", [Message]),
			receive_spawned_cnode(Erlang_Port)
	end.

-spec receive_cnode_response() ->
				any().
%% @doc
%% .
%% @end
receive_cnode_response() ->
	receive
		{erl_i2c_cnode, Message} ->
			Message;
		Message ->
			error_logger:warning_msg("unknown format in answer:~n~p~n", [Message]),
			{error}
	end.

-spec send_cnode(
				Nodename::atom(),
				Message::term()) ->
				any().
%% @doc
%%
%% @end
send_cnode(Nodename, Message) ->
	{any, Nodename} ! {call, self(), Message}.

% vim:ft=erlang shiftwidth=2 tabstop=2 softtabstop=2
