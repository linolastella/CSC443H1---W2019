@ECHO OFF
REM Lino Lastella 1001237654 lastell1
REM
REM This is a batch script to run the first program on all possible combinations
REM of inputs as requested from the handout.
REM
REM B is the number of buffer pages, P is the data page size, F is the field to sort by
REM
REM Must be run from executable's parent directory

SET /A i=0

SETLOCAL EnableDelayedExpansion

FOR %%B IN (3, 10, 20, 50, 100, 200, 500, 1000, 5000, 10000) DO (
    FOR %%P IN (512, 1024, 2048) DO (
        FOR  %%F IN (1) DO (
            SET /A i=i+1
            START /WAIT /B build\external_sorting.exe names.db sorted_names_!i!.db %%B %%P %%F
            @ECHO:
        )
    )
)
