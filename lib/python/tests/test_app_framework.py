import unittest

from satos_payload import app_framework


class TestStoppable(unittest.TestCase):

    def test_not_stopped(self):
        st = app_framework.Stoppable()
        self.assertTrue(st.stopped)
