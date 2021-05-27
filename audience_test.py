import pytest
import subprocess

###################
audienceApp = "./audience.out"
testFilesPath = "resources/"

def run_audience(infile, outfile):
    result = subprocess.run([audienceApp, testFilesPath+infile, testFilesPath+outfile], check=False)
    return result.returncode

def run_audience1(infile):
    result = subprocess.run([audienceApp, testFilesPath+infile], check=False)
    return result.returncode

def run_audience0():
    result = subprocess.run(audienceApp, check=False)
    return result.returncode

def compare_files(outfile, expectedfile):
    # to ignore newline chars
    out = open(testFilesPath+outfile, 'r').read()
    exp = open(testFilesPath+expectedfile, 'r').read()
    return (out == exp)

###################


def test_audience_no_in_file():
    assert run_audience0() == 1

def test_audience_no_out_file():
    assert run_audience1("input-statements.psv") == 1

def test_audience_in_file_not_exist():
    assert run_audience1("fake-in-file") == 1

def test_audience_in_file_not_exist2():
    assert run_audience("fake-in-file", "out-file-no-matter") == 1

def test_audience_defult():
    assert run_audience("input-statements.psv", "output-statements.psv") == 0
    assert compare_files("expected-sessions.psv", "output-statements.psv") == True

def test_audience_many_days():
    assert run_audience("input-many-days.psv", "output-many-days.psv") == 0
    assert compare_files("expected-many-days.psv", "output-many-days.psv") == True

def test_audience_broken_in():
    assert run_audience("input-broken.psv", "output-broken.psv") == 0
    assert compare_files("expected-broken.psv", "output-broken.psv") == True

def test_audience_duplicated_entries():
    assert run_audience("input-duplicated-entries.psv", "output-duplicated-entries.psv") == 0
    assert compare_files("expected-duplicated-entries.psv", "output-duplicated-entries.psv") == True

def test_audience_date_invalid():
    assert run_audience("input-date-invalid.psv", "output-date-invalid.psv") == 0
    assert compare_files("expected-date-invalid.psv", "output-date-invalid.psv") == True