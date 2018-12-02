\
\ This file and its contents are supplied under the terms of the
\ Common Development and Distribution License ("CDDL"), version 1.0.
\ You may only use this file in accordance with the terms of version
\ 1.0 of the CDDL.
\
\ A full copy of the text of the CDDL should have accompanied this
\ source.  A copy of the CDDL is also available via the Internet at
\ http://www.illumos.org/license/CDDL.

\ Copyright 2017 Toomas Soome <tsoome@me.com>

\ This module is implementing the beadm user command to support listing
\ and switching Boot Environments (BE) from command line and
\ support words to provide data for BE menu in loader menu system.
\ Note: this module needs an update to provide proper BE vocabulary.

only forth also support-functions also file-processing
also file-processing definitions also parser
also line-reading definitions also builtins definitions

variable page_count
variable page_remainder
0 page_count !
0 page_remainder !

\ from menu.4th
: +c! ( N C-ADDR/U K -- C-ADDR/U )
	3 pick 3 pick	( n c-addr/u k -- n c-addr/u k n c-addr )
	rot + c!	( n c-addr/u k n c-addr -- n c-addr/u )
	rot drop	( n c-addr/u -- c-addr/u )
;

: get_value ( -- )
	eat_space
	line_pointer
	skip_to_end_of_line
	line_pointer over -
	strdup value_buffer strset
	['] exit to parsing_function
;

: get_name ( -- )
	read_name
	['] get_value to parsing_function
;

: get_name_value
	line_buffer strget + to end_of_line
	line_buffer .addr @ to line_pointer
	['] get_name to parsing_function
	begin
		end_of_line? 0=
	while
		parsing_function execute
	repeat
;

\ beadm support
: beadm_longest_title ( addr len -- width )
	0 to end_of_file?
	O_RDONLY fopen fd !
	reset_line_reading
	fd @ -1 = if EOPEN throw then
	0 >r		\ length into return stack
	begin
		end_of_file? 0=
	while
		free_buffers
		read_line
		get_name_value
		value_buffer .len @ r@ > if r> drop value_buffer .len @ >r then
		free_buffers
		read_line
	repeat
	fd @ fclose
	r> 1 +		\ space between columns
;

\ Pretty print BE list
: beadm_list ( width addr len -- )
	0 to end_of_file?
	O_RDONLY fopen fd !
	reset_line_reading
	fd @ -1 = if EOPEN throw then
	." BE" dup 2 - spaces ." Type    Device" cr
	begin
		end_of_file? 0=
	while
		free_buffers
		read_line
		get_name_value
		value_buffer strget type
		dup value_buffer .len @ - spaces
		free_buffers
		read_line
		get_name_value
		name_buffer strget type
		name_buffer strget s" bootfs" compare 0= if 2 spaces then
		name_buffer strget s" chain" compare 0= if 3 spaces then
		value_buffer strget type cr
		free_buffers
	repeat
	fd @ fclose
	drop
;

\ we are called with strings be_name menu_file, to simplify the stack
\ management, we open the menu and free the menu_file.
: beadm_bootfs ( be_addr be_len maddr mlen -- addr len taddr tlen flag | flag )
	0 to end_of_file?
	2dup O_RDONLY fopen fd !
	drop free-memory
	fd @ -1 = if EOPEN throw then
	reset_line_reading
	begin
		end_of_file? 0=
	while
		free_buffers
		read_line
		get_name_value
		2dup value_buffer strget compare
		0= if ( title == be )
			2drop		\ drop be_name
			free_buffers
			read_line
			get_name_value
			value_buffer strget strdup
			name_buffer strget strdup -1
			free_buffers
			1 to end_of_file? \ mark end of file to skip the rest
		else
			read_line	\ skip over next line
		then
	repeat
	fd @ fclose
	line_buffer strfree
	read_buffer strfree
	dup -1 > if ( be_addr be_len )
		2drop
		0
	then
;

: current-dev ( -- addr len ) \ return current dev
	s" currdev" getenv
	2dup [char] / strchr nip
	dup 0> if ( strchr '/' != NULL ) - else drop then
	\ we have now zfs:pool or diskname:
;

\ chop trailing ':'
: colon- ( addr len -- addr len - 1 | addr len )
	2dup 1 - + C@ [char] : = if ( string[len-1] == ':' ) 1 - then
;

\ add trailing ':'
: colon+ ( addr len -- addr len+1 )
	2dup +			\ addr len -- addr+len
	[char] : swap c!	\ save ':' at the end of the string
	1+			\ addr len -- addr len+1
;

\ make menu.lst path
: menu.lst ( addr len -- addr' len' )
	colon-
	\ need to allocate space for len + 16
	dup 16 + allocate if ENOMEM throw then
	swap 2dup 2>R	\ copy of new addr len to return stack
	move 2R>
	s" :/boot/menu.lst" strcat
;

\ list be's on device
: list-dev ( addr len -- )
	menu.lst 2dup 2>R
	beadm_longest_title
	line_buffer strfree
	read_buffer strfree
	R@ swap 2R>	\ addr width addr len
	beadm_list free-memory
	." Current boot device: " s" currdev" getenv type cr
	line_buffer strfree
	read_buffer strfree
;

\ activate be on device.
\ if be name was not given, set currdev
\ otherwize, we query device:/boot/menu.lst for bootfs and
\ if found, and bootfs type is chain, attempt chainload.
\ set currdev to bootfs.
\ if we were able to set currdev, reload the config

: activate-dev ( dev.addr dev.len be.addr be.len -- )

	dup 0= if
		2drop
		colon-			\ remove : at the end of the dev name
		dup 1+ allocate if ENOMEM throw then
		dup 2swap 0 -rot strcat
		colon+
		s" currdev" setenv	\ setenv currdev = device
		free-memory
	else
		2swap menu.lst
		beadm_bootfs if ( addr len taddr tlen )
			2dup s" chain" compare 0= if
				drop free-memory	\ free type
				2dup
				dup 6 + allocate if ENOMEM throw then
				dup >R
				0 s" chain " strcat
				2swap strcat ['] evaluate catch drop
				\ We are still there?
				R> free-memory		\ free chain command
				drop free-memory	\ free addr
				exit
			then
			drop free-memory		\ free type
			\ check last char in the name
			2dup + c@ [char] : <> if
				\ have dataset and need to get zfs:pool/ROOT/be:
				dup 5 + allocate if ENOMEM throw then
				0 s" zfs:" strcat
				2swap strcat
				colon+
			then
			2dup s" currdev" setenv
			drop free-memory
		else
			." No such BE in menu.lst or menu.lst is missing." cr
			exit
		then
	then

	\ reset BE menu
	0 page_count !
	\ need to do:
	0 unload drop
	free-module-options
	\ unset the env variables with kernel arguments
	s" acpi-user-options" unsetenv
	s" boot-args" unsetenv
	s" boot_ask" unsetenv
	s" boot_single" unsetenv
	s" boot_verbose" unsetenv
	s" boot_kmdb" unsetenv
	s" boot_debug" unsetenv
	s" boot_reconfigure" unsetenv
	start			\ load config, kernel and modules
	." Current boot device: " s" currdev" getenv type cr
;

\ beadm list [device]
\ beadm activate BE [device] | device
\
\ lists BE's from current or specified device /boot/menu.lst file
\ activates specified BE by unloading modules, setting currdev and
\ running start to load configuration.
: beadm ( -- ) ( throws: abort )
	0= if ( interpreted ) get_arguments then

	dup 0= if
		." Usage:" cr
		." beadm activate {beName [device] | device}" cr
		." beadm list [device]" cr
		." Use lsdev to get device names." cr
		drop exit
	then
	\ First argument is 0 when we're interprated.  See support.4th
	\ for get_arguments reading the rest of the line and parsing it
	\ stack: argN lenN ... arg1 len1 N
	\ rotate arg1 len1, dont use argv[] as we want to get arg1 out of stack
	-rot 2dup

	s" list" compare-insensitive 0= if ( list )
		2drop
		argc 1 = if ( list currdev )
			\ add dev to list of args and switch to case 2
			current-dev rot 1 +
		then
		2 = if ( list device ) list-dev exit then
		." too many arguments" cr abort
	then
	s" activate" compare-insensitive 0= if ( activate )
		argc 1 = if ( missing be )
			drop ." missing bName" cr abort
		then
		argc 2 = if ( activate be )
			\ need to set arg list into proper order
			1+ >R	\ save argc+1 to return stack

			\ if the prefix is fd, cd, net or disk and we have :
			\ in the name, it is device and inject empty be name
			over 2 s" fd" compare 0= >R
			over 2 s" cd" compare 0= R> or >R
			over 3 s" net" compare 0= R> or >R
			over 4 s" disk" compare 0= R> or
			if ( prefix is fd or cd or net or disk )
				2dup [char] : strchr nip
				if ( its : in name )
					true
				else
					false
				then
			else
				false
			then

			if ( it is device name )
				0 0 R>
			else
				\ add device, swap with be and receive argc
				current-dev 2swap R>
			then
		then
		3 = if ( activate be device ) activate-dev exit then
		." too many arguments" cr abort
	then
	." Unknown argument" cr abort
;

also forth definitions also builtins

\ make beadm available as user command.
builtin: beadm

\ count the pages of BE list
\ leave FALSE in stack in case of error
: be-pages ( -- flag )
	1 local flag
	0 0 2local currdev
	0 0 2local title
	end-locals

	current-dev menu.lst 2dup 2>R
	0 to end_of_file?
	O_RDONLY fopen fd !
	2R> drop free-memory
	reset_line_reading
	fd @ -1 = if FALSE else
		s" currdev" getenv
		over			( addr len addr )
		4 s" zfs:" compare 0= if
			5 -			\ len -= 5
			swap 4 +		\ addr += 4
			swap to currdev
		then

		0
		begin
			end_of_file? 0=
		while
			read_line
			get_name_value
			s" title" name_buffer strget compare
			0= if 1+ then

			flag if		\ check for title
				value_buffer strget strdup to title free_buffers
				read_line		\ get bootfs
				get_name_value
				value_buffer strget currdev compare 0= if
					title s" zfs_be_active" setenv
					0 to flag
				then
				title drop free-memory 0 0 to title
				free_buffers
			else
				free_buffers
				read_line		\ get bootfs
			then
		repeat
		fd @ fclose
		line_buffer strfree
		read_buffer strfree
		5 /mod swap dup page_remainder !		\ save remainder
		if 1+ then
		dup page_count !				\ save count
		s>d <# #s #> s" zfs_be_pages" setenv
		TRUE
	then
;

: be-set-page { | entry count n device -- }
	page_count @ 0= if
		be-pages
		page_count @ 0= if exit then
	then

	0 to device
	s" zfs_be_currpage" getenv dup -1 = if
		drop s" 1"
	then
	0 s>d 2swap
	>number ( ud caddr/u -- ud' caddr'/u' )
	2drop
	1 um/mod nip 5 *
	page_count @ 5 *
	page_remainder @ if
		5 page_remainder @ - -
	then
	swap -
	dup to entry
	0 < if
		entry 5 + to count
		0 to entry
	else
		5 to count
	then
	current-dev menu.lst 2dup 2>R
	0 to end_of_file?
	O_RDONLY fopen fd !
	2R> drop free-memory
	reset_line_reading
	fd @ -1 = if EOPEN throw then
	0 to n
	begin
		end_of_file? 0=
	while
		n entry < if
			read_line		\ skip title
			read_line		\ skip bootfs
			n 1+ to n
		else
			\ Use reverse loop to display descending order
			\ for BE list.
			0 count 1- do
				read_line		\ read title line
				get_name_value
				value_buffer strget
				52 i +			\ ascii 4 + i
				s" bootenvmenu_caption[4]" 20 +c! setenv
				value_buffer strget
				52 i +			\ ascii 4 + i
				s" bootenvansi_caption[4]" 20 +c! setenv

				free_buffers
				read_line		\ read value line
				get_name_value

				\ set menu entry command
				name_buffer strget s" chain" compare
				0= if
					s" set_be_chain"
				else
					s" set_bootenv"
				then
				52 i +			\ ascii 4 + i
				s" bootenvmenu_command[4]" 20 +c! setenv

				\ set device name
				name_buffer strget s" chain" compare
				0= if
					\ for chain, use the value as is
					value_buffer strget
				else
					\ check last char in the name
					value_buffer strget 2dup + c@
					[char] : <> if
						\ make zfs device name
						swap drop
						5 + allocate if
							ENOMEM throw
						then
						s" zfs:" ( addr addr' len' )
						2 pick swap move ( addr )
						dup to device
						4 value_buffer strget
						strcat	( addr len )
						s" :" strcat
					then
				then

				52 i +			\ ascii 4 + i
				s" bootenv_root[4]" 13 +c! setenv
				device free-memory 0 to device
				free_buffers
				-1
			+loop

			5 count do		\ unset unused entries
				52 i +			\ ascii 4 + i
				dup s" bootenvmenu_caption[4]" 20 +c! unsetenv
				dup s" bootenvansi_caption[4]" 20 +c! unsetenv
				dup s" bootenvmenu_command[4]" 20 +c! unsetenv
				s" bootenv_root[4]" 13 +c! unsetenv
			loop

			1 to end_of_file?		\ we are done
		then
	repeat
	fd @ fclose
	line_buffer strfree
	read_buffer strfree
;
