

    JUMP 1.0

    (c) 1994 by Thomas Opheys
    opheys@kirk.fmi.uni-passau.de
    all rights reserved



    OVERVIEW

    JUMP implements jump-scrolling in OS/2 sessions. Currently, the only
    programs that can make use of jump-scrolling are the ones that only
    use stdin, stdout and stderr I/O.



    WHAT CAN BE DONE

    The following paragraphs show how to speed up scrolling of a
    program's output significantly. As an example, the 'dir' command
    is used, but generally all other internal and external commands
    or programs can be accelerated.


    1. 4OS/2 ALIAS

    If you have installed 4OS/2 and use it as your command interpreter,
    you can set up an alias for 'dir' to use jump scrolling:

    alias dir=jump *dir

    Include the above line in your 4START.CMD.


    2. BATCH FILE

    You can create a batch file for every command. Note that this
    method has two disadvantages in contrast to an alias: first,
    it is a little slower because a new shell is started to interpret
    the batch file. Second, you can't name the batch file 'dir' or
    'dir.cmd'.

    @jump dir %1 %2 %3 %4 %5 %6 %7 %8 %9

    If the line above is written to a file 'JD.CMD' in your PATH,
    you can use the faster 'jd' command instead of 'dir'. There even
    is a small advantage: if you expect a short output of 'dir',
    you don't have to use jump scrolling. For longer listings, use 'jd'.


    3. DIRECT INVOCATION

    If you don't want to set up aliases or batch files or if you want
    to use jump-scrolling with infrequently used programs, you can
    directly invoke a program with

    [C:\] jump <program> <options>

    For example, type 'jump dir \ /s /p' instead of 'dir \ /s /p'.


    4. JUMP-SCROLLING SHELL

    If you can live with the limitations described below, this is the
    fastest method for using jump-scrolling, even using the least amount
    of ressources:

    Setup a program object for 'JUMP.EXE' and specify 'CMD.EXE' as the
    parameter. This objects start a fully jump-scrolling shell.

    Limitations: if you use programs that write to the screen via old
    16-bit VIO-Functions, chances are good that the display gets
    scrambled. While the standard OS/2 'CMD.EXE' command interpreter
    works with JUMP, the 4OS/2 shell does not. 4OS/2 does a lot of
    VIO-calls, beginning right with the prompt. So you have to live with
    CMD.EXE and its limitations or use one of the methods above. One
    good idea, though, is to use a 'JUMP CMD'-Shell for simple 'dir',
    'cd', 'type', ... commands only. Watch the scrolling speed!


    WHAT CAN NOT BE DONE

    Programs directly writing to the screen cannot use jump-scrolling.
    Even more, they generally scramble the contents of the screen when
    run with JUMP.


    INSTALLATION

    - copy JUMP.EXE to a directory in your PATH
    - create the necessary aliases (change/start/use 4alias.btm),
      batch files or program object
    - optionally set the JUMP_SHELL environment variable if you want
      JUMP to use another shell than specified by OS2_SHELL or COMSPEC


    MISCELLANEOUS

    - When you first start JUMP, it preloads itself once and stays resident
      in memory so that further starts of JUMP will be faster. This preloaded
      JUMP process will stay in memory until you kill it or reboot.

    - If you own an IBM C compiler and if you use OS/2 WARP v3, you can
      recompile JUMP to create a smaller executable ('make ibmwarp').

    - The "emx" target of the makefile currently compiles a version of
      JUMP which doesn't preload itself into memory.

    - The environment variables JUMP_SHELL or, if not found, OS2_SHELL or,
      if not found, COMSPEC will be used to get the name of the command
      interpreter to use. If parameters follow the EXE file name, these
      are ONLY passed to the shell if you start JUMP without any parameters.
      If you start jump with parameter(s), only '/C '+parameter(s) is
      passed to the shell.

    - JUMP currently ignores any screen color when scrolling. Characters
      with different foreground/background colors are scrolled upwards but
      the color attributes stay at the same screen position, now changing
      the color of other characters. If there is need for color support,
      just tell me.

    - JUMP currently supports the following control characters:
      BEL (Bell), BS (backspace), TAB (tabulator), LF (line feed),
      CR (carriage return) and any ANSI escape sequences. All other
      characters are printed directly. TABs and ANSI ESC-sequences
      are passed to the system file handle to be processed as normal.

    - the command 'JUMP /?' can't be redirected! This is NOT a bug... :-)


    COPYRIGHT

    All files in this archive, especially jump.c and jump.exe are
    (C) 1994 by Thomas S. Opheys; all rights reserved.

    If you can make use of JUMP, just do it (no, I'm not sponsored by NIKE).
    Any contributions to JUMP are very welcome, especially corrected bugs
    and additional features. Please send modifications via email to:

    opheys@kirk.fmi.uni-passau.de

    Please don't repackage and redistribute your modified versions...
    You may use parts of the JUMP source code for your own programs, but
    only if your work is noncommercial. Please send me your o(pi)nions!


    Thomas S. Opheys
    Franz-Stockbauer-Weg 1/88
    94032 Passau
    Germany
    +49 851 73971

