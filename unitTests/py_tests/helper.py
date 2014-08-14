#!/usr/bin/env python3
# -*- encoding: utf-8 -*-

from __future__ import print_function

import math

import pytest


SCUFF_CASPOL_CMD = "scuff-caspol "
DATA = "./data"

RELTOL = 0.1  # relative tolerance
RELTOL_VALIDATE = 0.01  # relative tolerance for validation code


def read_file(file_):
    """
    Read byXi files that are produced by some scuff-em function.

    .. note::

        If a numpy dependency is introduced this code becomes obsolete.

    Returns
    -------
    Returns a list of tuples of data.

    """
    datas = list()
    for line in file_:
        if line[0] == "#":
            continue
        datas.append([float(number) for number in line.strip().split(" ")])
    return datas


def assert_rel_tol(scuff, theory, reltol=RELTOL):
    """
    asserts if the scuff number and the theory number are within the relative
    tolerance reltol.

    """
    #__tracebackhide__ = True
    quotient = scuff / theory
    if quotient <= 0:  # different sign
        pytest.fail("different sign")
    elif abs(math.log(quotient)) > math.log(1.0 + reltol):
        pytest.fail("values differ beyond allowed tolerance")

