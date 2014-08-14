#!/usr/bin/env python3
# -*- encoding: utf-8 -*-

"""
Unit tests for scuff-caspol that can be used with py.test.

They should run in under 10 seconds.

For details see README.rst in unitTests/py_tests

"""

from __future__ import print_function

import os

import pytest

from helper import (SCUFF_CASPOL_CMD, RELTOL_VALIDATE, read_file,
                    assert_rel_tol)

slow = pytest.mark.slow
validation = pytest.mark.validation


@pytest.fixture(scope="module")
def pecplate_fast(create_temp):
    os.system(SCUFF_CASPOL_CMD + "--EPFILE dist.txt --atom Rubidium --PECPlate --XiFile freq.txt")
    return {"byXi": "PECPlate.byXi"}


@pytest.fixture(scope="module")
def pecplate_validate(create_temp):
    os.system(SCUFF_CASPOL_CMD + "--EPFILE distvalidate.txt --atom Rubidium --PECPlate --Temperature 1")
    return {"byXi": "PECPlate.byXi",
            "out": "PECPlate.out"}


@pytest.fixture(scope="module")
def tetrahedron_pec(create_temp):
    os.system(SCUFF_CASPOL_CMD + "--EPFILE dist.txt --atom Rubidium --geometry tetrahedron_pec_6.scuffgeo --XiFile freq.txt")
    return {"byXi": "tetrahedron_pec_6.byXi"}


@pytest.fixture(scope="module")
def tetrahedron_eps10(create_temp):
    os.system(SCUFF_CASPOL_CMD + "--EPFILE dist.txt --atom Rubidium --geometry tetrahedron_eps10_6.scuffgeo --Xi 6.0")
    return {"byXi": "tetrahedron_eps10_6.byXi"}


@pytest.fixture(scope="module")
def coin(create_temp):
    os.system(SCUFF_CASPOL_CMD + "--EPFILE dist.txt --atom Rubidium --geometry coin_pec_144.scuffgeo --XiFile freq.txt")
    return {"byXi": "coin_pec_144.byXi"}


@pytest.fixture(scope="module")
def thinplate(create_temp):
    os.system(SCUFF_CASPOL_CMD + "--EPFILE distvalidate.txt --atom Rubidium --geometry thinplate_pec.scuffgeo --Temperature 1")
    return {"byXi": "thinplate_pec.byXi",
            "out": "thinplate_pec.out"}

def validate_long_short_range(datas):
    r"""
    Short range and long range approximations are taken from [1]_ with
    :math:`C^{SR} = 0.0138 neV \mum^3` and :math:`C^{LR} = 8.83e-5 neV \mum^4`
    The corresponding potentials can be obtained by

    .. math::

        U^{SR}(z) = - \frac{C^{SR}}{z^3}
        U^{LR}(z) = - \frac{C^{LR}}{z^4}

    .. [1] `http://homerreid.dyndns.org/scuff-EM/scuff-caspol/scuff-caspol-Tutorial.shtml`_

    """
    def u_sr(zz):
        return -0.0138 / zz**3
    def u_lr(zz):
        return -8.83e-5 / zz**4
    for xx, yy, zz, cp_pot in datas:
        if zz <= 0.001:
            assert_rel_tol(cp_pot, u_sr(zz), RELTOL_VALIDATE)
        elif zz >= 0.02:
            assert_rel_tol(cp_pot, u_lr(zz), RELTOL_VALIDATE)
        else:
            # not testable
            pass


@validation
def test_thinplate(thinplate):
    with open(thinplate["out"], "r") as out_file:
        datas = read_file(out_file)
    validate_long_short_range(datas)


@validation
def test_pecplate(pecplate_validate):
    with open(pecplate_validate["out"], "r") as out_file:
        datas = read_file(out_file)
    validate_long_short_range(datas)


@slow
def test_coin_xi0(coin):
    with open(coin["byXi"], "r") as out_file:
        datas = read_file(out_file)
    for xx, yy, zz, xi, polarize, cp_pot in datas:
        if xi == 1.0e-6:
            assert_rel_tol(polarize, 3.186000e+02)
            if zz == 0.1:
                assert_rel_tol(cp_pot, -6.313080e-01)
            elif zz == 1.0:
                assert_rel_tol(cp_pot, -1.248351e-04)
            elif zz == 10.0:
                assert_rel_tol(cp_pot, -2.273920e-10)
            else:
                raise ValueError("TEST: no comparison value for zz={zz:e}, xi={xi:e}".format(zz=zz, xi=xi))


@slow
def test_coin_xi6(coin):
    with open(coin["byXi"], "r") as out_file:
        datas = read_file(out_file)
    for xx, yy, zz, xi, polarize, cp_pot in datas:
        if xi == 6.0:
            assert_rel_tol(polarize, 2.074813e+02)
            if zz == 0.1:
                assert_rel_tol(cp_pot, -3.599895e-01)
            elif zz == 1.0:
                assert_rel_tol(cp_pot, -1.923850e-07)
            elif zz == 10.0:
                assert_rel_tol(cp_pot, -3.601550e-56)
            else:
                raise ValueError("TEST: no comparison value for zz={zz:e}, xi={xi:e}".format(zz=zz, xi=xi))


def test_tetrahedron_pec_xi0(tetrahedron_pec):
    with open(tetrahedron_pec["byXi"], "r") as out_file:
        datas = read_file(out_file)
    for xx, yy, zz, xi, polarize, cp_pot in datas:
        if xi == 1.0e-6:
            assert_rel_tol(polarize, 3.186000e+02)
            if zz == 0.1:
                assert_rel_tol(cp_pot, -2.029004e-01)
            elif zz == 1.0:
                assert_rel_tol(cp_pot, -2.493534e-06)
            elif zz == 10.0:
                assert_rel_tol(cp_pot, -3.243498e-12)
            else:
                raise ValueError("TEST: no comparison value for zz={zz:e}, xi={xi:e}".format(zz=zz, xi=xi))



def test_tetrahedron_pec_xi6(tetrahedron_pec):
    with open(tetrahedron_pec["byXi"], "r") as out_file:
        datas = read_file(out_file)
    for xx, yy, zz, xi, polarize, cp_pot in datas:
        if xi == 6.0:
            assert_rel_tol(polarize, 2.074813e+02)
            if zz == 0.1:
                assert_rel_tol(cp_pot, -1.168001e-01)
            elif zz == 1.0:
                assert_rel_tol(cp_pot, -6.200486e-09)
            elif zz == 10.0:
                assert_rel_tol(cp_pot, -6.149101e-58)
            else:
                raise ValueError("TEST: no comparison value for zz={zz:e}, xi={xi:e}".format(zz=zz, xi=xi))



def test_tetrahedron_eps10_xi6(tetrahedron_eps10):
    with open(tetrahedron_eps10["byXi"], "r") as out_file:
        datas = read_file(out_file)
    for xx, yy, zz, xi, polarize, cp_pot in datas:
        if xi == 6.0:
            assert_rel_tol(polarize, 2.074813e+02)
            if zz == 0.1:
                assert_rel_tol(cp_pot, -7.722056e-02)
            elif zz == 1.0:
                assert_rel_tol(cp_pot, -3.197261e-09)
            elif zz == 10.0:
                assert_rel_tol(cp_pot, -3.089228e-58)
            else:
                raise ValueError("TEST: no comparison value for zz={zz:e}, xi={xi:e}".format(zz=zz, xi=xi))


def test_infinite_plate_xi0(pecplate_fast):
    with open(pecplate_fast["byXi"], "r") as out_file:
        datas = read_file(out_file)
    for xx, yy, zz, xi, polarize, cp_pot in datas:
        if xi == 1.0e-6:
            assert_rel_tol(polarize, 3.186000e+02)
            if zz == 0.1:
                assert_rel_tol(cp_pot, -7.418665e-01)
            elif zz == 1.0:
                assert_rel_tol(cp_pot, -7.418665e-04)
            elif zz == 10.0:
                assert_rel_tol(cp_pot, -7.418665e-07)
            else:
                raise ValueError("TEST: no comparison value for zz={zz:e}, xi={xi:e}".format(zz=zz, xi=xi))


def test_infinite_plate_xi6(pecplate_fast):
    with open(pecplate_fast["byXi"], "r") as out_file:
        datas = read_file(out_file)
    for xx, yy, zz, xi, polarize, cp_pot in datas:
        if xi == 6.0:
            assert_rel_tol(polarize, 2.074813e+02)
            if zz == 0.1:
                assert_rel_tol(cp_pot, -4.249017e-01)
            elif zz == 1.0:
                assert_rel_tol(cp_pot, -2.523156e-07)
            elif zz == 10.0:
                assert_rel_tol(cp_pot, -2.712012e-55)
            else:
                raise ValueError("TEST: no comparison value for zz={zz:e}, xi={xi:e}".format(zz=zz, xi=xi))

