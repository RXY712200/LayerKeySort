"""Regression: candidate ZIP bytes must not depend on Git newline configuration."""
import datetime
import importlib.util
import os
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[1] / "mini_release_assets.py"
spec = importlib.util.spec_from_file_location("mini_release_assets", SCRIPT)
assets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assets)


class ArchiveNewlineTest(unittest.TestCase):
    def test_archive_timestamp_matches_source_commit(self):
        commit, _, _ = assets.snapshot("HEAD")
        timestamp = int(assets.git("show", "-s", "--format=%ct", commit))
        date = datetime.datetime.fromtimestamp(timestamp, datetime.timezone.utc)
        expected = (date.year, date.month, date.day, date.hour, date.minute,
                    date.second - date.second % 2)
        with tempfile.TemporaryDirectory(prefix="mini-mtime-regression-") as temp:
            directory = Path(temp) / "assets"
            result = assets.build(directory, commit)
            with zipfile.ZipFile(directory / result[2]) as archive:
                for member in archive.infolist():
                    self.assertEqual(member.date_time, expected, member.filename)

    def test_lf_and_crlf_git_configuration_produce_identical_verified_assets(self):
        with tempfile.TemporaryDirectory(prefix="mini-newline-regression-") as temp:
            results = []
            for label, autocrlf, eol in [("lf", "false", "lf"), ("crlf", "true", "crlf")]:
                environment = {
                    "TZ": "UTC" if label == "lf" else "GMT-8",
                    "GIT_CONFIG_COUNT": "2",
                    "GIT_CONFIG_KEY_0": "core.autocrlf",
                    "GIT_CONFIG_VALUE_0": autocrlf,
                    "GIT_CONFIG_KEY_1": "core.eol",
                    "GIT_CONFIG_VALUE_1": eol,
                }
                directory = Path(temp) / label
                with patch.dict(os.environ, environment):
                    result = assets.build(directory, "HEAD")
                    self.assertEqual(result, assets.verify(directory, result[0]))
                results.append({p.name: p.read_bytes() for p in directory.iterdir()})
            self.assertEqual(results[0], results[1])


if __name__ == "__main__":
    unittest.main(verbosity=2)
