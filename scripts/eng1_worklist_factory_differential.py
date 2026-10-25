"""Worklist factory differential entry point.

Use unpatched native instructions in Unicorn plus source-only prefix guards.
Historical OS-module-loading receipts remain dated evidence; this entry point
never loads an original module or starts the game.
"""
from worklist_live_reads_differential import main

if __name__ == '__main__':
    main()
