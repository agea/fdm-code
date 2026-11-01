"""Stop two-process test clients without racing their return to the menu."""
from pathlib import Path
import time

MENU_LOADED = 'Load map complete /Game/EclipseRecoil/Maps/L_MainMenu'


def settle_clients(processes, logs, timeout=15):
    """Wait for clients that lost their host; index 0 of both lists is the host.

    Losing the host sends a client through a blocking menu load. SIGTERM inside
    that load crashes it on exit and leaves CrashReportClient holding its sockets.
    """
    if not processes or processes[0].poll() is None:
        return
    waiting = list(zip(processes[1:], logs[1:]))
    deadline = time.monotonic() + timeout
    while True:
        waiting = [(process, log) for process, log in waiting
                   if process.poll() is None and MENU_LOADED not in Path(log.name).read_text(errors='replace')]
        if not waiting or time.monotonic() >= deadline:
            return
        time.sleep(.2)
