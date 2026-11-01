import importlib.util
from pathlib import Path
import tempfile
import time
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location('shutdown', Path(__file__).parents[1] / 'client_shutdown.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Process:
    def __init__(self, running):
        self.running = running

    def poll(self):
        return None if self.running else 0


class SettleClientsTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.logs = [(Path(directory.name) / name).open('w') for name in ('host.log', 'client.log')]
        for log in self.logs:
            self.addCleanup(log.close)

    def settle(self, host, client, timeout=0):
        with mock.patch.object(module.time, 'sleep') as sleep:
            module.settle_clients([host, client], self.logs, timeout)
        return sleep.call_count

    def test_running_host_means_no_menu_load_to_wait_for(self):
        self.assertEqual(self.settle(Process(True), Process(True), timeout=60), 0)

    def test_loaded_menu_releases_the_client(self):
        self.logs[1].write(module.MENU_LOADED + '\n')
        self.logs[1].flush()
        self.assertEqual(self.settle(Process(False), Process(True), timeout=60), 0)

    def test_exited_client_needs_no_menu(self):
        self.assertEqual(self.settle(Process(False), Process(False), timeout=60), 0)

    def test_missing_menu_gives_up_at_the_deadline(self):
        started = time.monotonic()
        self.settle(Process(False), Process(True))
        self.assertLess(time.monotonic() - started, 5)

    def test_no_processes_is_a_no_op(self):
        module.settle_clients([], [])


if __name__ == '__main__':
    unittest.main()
