// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2008, 2009, 2010, 2011 LSST Corporation.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#include "lsst/shapelet/ShapeletFunction.h"
#include "lsst/shapelet/GaussHermiteConvolution.h"
#include "lsst/afw/geom/Angle.h"
#include "ndarray/eigen.h"

namespace lsst { namespace shapelet {

namespace {

class TripleProductIntegral {
public:

    ndarray::Array<double,3,3> const asArray() const { return _array; }

    static ndarray::Array<double,3,3> make1d(int const order1, int const order2, int const order3);

    TripleProductIntegral(int order1, int order2, int order3);

    ndarray::Vector<int,3> const & getOrders() const { return _orders; }

private:

    ndarray::Vector<int,3> _orders;
    ndarray::Array<double,3,3> _array;
};

TripleProductIntegral::TripleProductIntegral(int order1, int order2, int order3) :
    _orders(ndarray::makeVector(order1, order2, order3)),
    _array(
        ndarray::allocate(
            ndarray::makeVector(
                computeOffset(order1+1),
                computeOffset(order2+1),
                computeOffset(order3+1)
            )
        )
    )
{
    _array.deep() = 0.0;
    ndarray::Array<double,3,3> a1d = make1d(order1, order2, order3);
    ndarray::Array<double,3,3>::Index o, i, x, y, n;
    for (n[0] = o[0] = 0; n[0] <= _orders[0]; o[0] += ++n[0]) {
        for (n[2] = o[2] = 0; n[2] <= _orders[2]; o[2] += ++n[2]) {
            int n0n2 = n[0] + n[2];
            int max = (order2 < n0n2) ? _orders[1] : n0n2;
            for (o[1] = n[1] = bool(n0n2 % 2); n[1] <= max; (o[1] += ++n[1]) += ++n[1]) {
                for (i[0] = o[0], x[0] = 0, y[0] = n[0]; x[0] <= n[0]; ++x[0], --y[0], ++i[0]) {
                    for (i[2] = o[2], x[2] = 0, y[2] = n[2]; x[2] <= n[2]; ++x[2], --y[2], ++i[2]) {
                        for (i[1] = o[1], x[1] = 0, y[1] = n[1]; x[1] <= n[1]; ++x[1], --y[1], ++i[1]) {
                            _array[i] = a1d[x] * a1d[y];
                        }
                    }
                }
            }
        }
    }
}

ndarray::Array<double,3,3>
TripleProductIntegral::make1d(int order1, int order2, int order3) {
    ndarray::Array<double,3,3> array = ndarray::allocate(ndarray::makeVector(order1+1, order2+1, order3+1));
    array.deep() = 0.0;
    double first = array[ndarray::makeVector(0,0,0)] = BASIS_NORMALIZATION * M_SQRT1_2;
    if (order1 < 1) {
        if (order3 < 1) return array;
        // n1=0, n2=0, n3=even
        double previous = first;
        for (int n3 = 2; n3 <= order3; n3 += 2) {
            previous = array[ndarray::makeVector(0,0,n3)] = -std::sqrt((0.25 * (n3 - 1)) / n3) * previous;
        }
        // n1=0, n2>0, n3>0
        for (int n2 = 1; n2 <= order2; ++n2) {
            for (int n3 = n2; n3 <= order3; n3 += 2) {
                array[ndarray::makeVector(0,n2,n3)]
                    = std::sqrt((0.5 * n3) / n2) * array[ndarray::makeVector(0, n2-1, n3-1)];
            }
        }
        return array;
    }

    if (order3 < 1) {
        // n1=even, n2=0, n3=even
        double previous = first;
        for (int n1 = 2; n1 <= order1; n1 += 2) {
            previous = array[ndarray::makeVector(0,0,n1)] = -std::sqrt((0.25 * (n1 - 1)) / n1) * previous;
        }
        // n1=0, n2>0, n3>0
        for (int n2 = 1; n2 <= order2; ++n2) {
            for (int n1 = n2; n1 <= order1; n1 += 2) {
                array[ndarray::makeVector(n1,n2,0)]
                    = std::sqrt((0.5 * n1) / n2) * array[ndarray::makeVector(n1-1, n2-1, 0)];
            }
        }
        return array;
    }

    // n1=any, n2=0, n3=(0,1)
    {
        double previous = first;
        int n1 = 0;
        while (true) {
            if (++n1 > order1) break;
            array[ndarray::makeVector(n1, 0, 1)] = std::sqrt(0.25 * n1) * previous;
            if (++n1 > order1) break;
            previous = array[ndarray::makeVector(n1, 0, 0)] = -std::sqrt((0.25 * (n1 - 1)) / n1) * previous;
        }
    }

    // n1=(0,1), n2=0, n3=any
    {
        double previous = first;
        int n3 = 0;
        while (true) {
            if (++n3 > order3) break;
            array[ndarray::makeVector(1, 0, n3)] = std::sqrt(0.25 * n3) * previous;
            if (++n3 > order3) break;
            previous = array[ndarray::makeVector(0, 0, n3)] = -std::sqrt((0.25 * (n3 - 1)) / n3) * previous;
        }
    }

    // n1>1, n2=0, n3>1
    for (int n1 = 2; n1 <= order1; ++n1) {
        double f1 = -std::sqrt((0.25 * (n1 - 1)) / n1);
        for (int n3 = (n1 % 2) ? 3:2; n3 <= order3; n3 += 2) {
            array[ndarray::makeVector(n1,0,n3)]
                = f1 * array[ndarray::makeVector(n1-2, 0, n3)]
                + std::sqrt((0.25 * n3) / n1) * array[ndarray::makeVector(n1-1, 0, n3-1)];
        }
    }

    for (int n2 = 1; n2 <= order2; ++n2) {
        // n1>=n2, n3=0
        for (int n1 = n2; n1 <= order1; n1 += 2) {
            array[ndarray::makeVector(n1,n2,0)]
                = std::sqrt((0.5 * n1) / n2) * array[ndarray::makeVector(n1-1, n2-1, 0)];
        }
        // n1=0, n3>=n2
        for (int n3 = n2; n3 <= order3; n3 += 2) {
            array[ndarray::makeVector(0,n2,n3)]
                = std::sqrt((0.5 * n3) / n2) * array[ndarray::makeVector(0, n2-1, n3-1)];
        }
        // n1>0, n3>0
        for (int n1 = 1; n1 <= order1; ++n1) {
            for (int n3 = ((n1+n2) % 2) ? 1:2; n3 <= order3; n3 += 2) {
                array[ndarray::makeVector(n1,n2,n3)]
                    = std::sqrt((0.5 * n1) / n2) * array[ndarray::makeVector(n1-1, n2-1, n3)]
                    + std::sqrt((0.5 * n3) / n2) * array[ndarray::makeVector(n1, n2-1, n3-1)];
            }
        }
    }

    return array;
}

} // anonymous

class GaussHermiteConvolution::Impl {
public:

    Impl(int colOrder, ShapeletFunction const & psf);

    ndarray::Array<double const,2,2> evaluate(afw::geom::ellipses::Ellipse & ellipse) const;

    int getColOrder() const { return _colOrder; }

    int getRowOrder() const { return _rowOrder; }

private:
    int _rowOrder;
    int _colOrder;
    ShapeletFunction _psf;
    ndarray::Array<double,2,2> _result;
    TripleProductIntegral _tpi;
    HermiteTransformMatrix _htm;
};

GaussHermiteConvolution::Impl::Impl(
    int colOrder, ShapeletFunction const & psf
) :
    _rowOrder(colOrder + psf.getOrder()), _colOrder(colOrder), _psf(psf),
    _result(ndarray::allocate(computeSize(_rowOrder), computeSize(_colOrder))),
    _tpi(psf.getOrder(), _rowOrder, _colOrder),
    _htm(_rowOrder)
{
    _psf.changeBasisType(HERMITE);
}

ndarray::Array<double const,2,2> GaussHermiteConvolution::Impl::evaluate(
    afw::geom::ellipses::Ellipse & ellipse
) const {
    ndarray::EigenView<double,2,2> result(_result);
    ndarray::EigenView<double const,1,1> psf_coeff(_psf.getCoefficients());
    
    Eigen::Matrix2d psfT = _psf.getEllipse().getCore().getGridTransform().invert().getMatrix();
    Eigen::Matrix2d modelT = ellipse.getCore().getGridTransform().invert().getMatrix();
    ellipse.convolve(_psf.getEllipse()).inPlace();
    Eigen::Matrix2d convolvedTI = ellipse.getCore().getGridTransform().getMatrix() * std::sqrt(2.0);
    Eigen::Matrix2d psfArg = (convolvedTI * psfT).transpose();
    Eigen::Matrix2d modelArg = (convolvedTI * modelT).transpose();

    int const psfOrder = _psf.getOrder();

    Eigen::MatrixXd psfMat = _htm.compute(psfArg, psfOrder).adjoint();
    Eigen::MatrixXd modelMat = _htm.compute(modelArg, _colOrder).adjoint();

    // [kq]_m = \sum_m i^{n+m} [psfMat]_{m,n} [psf]_n
    // kq is zero unless {n+m} is even
    Eigen::VectorXd kq = Eigen::VectorXd::Zero(psfMat.size());
    for (int m = 0, mo = 0; m <= psfOrder; mo += ++m) {
        for (int n = m, no = mo; n <= psfOrder; (no += ++n) += ++n) {
            if ((n + m) % 4) { // (n + m) % 4 is always 0 or 2
                kq.segment(mo, m+1) -= psfMat.block(mo, no, m+1, n+1) * psf_coeff.segment(no, n+1);
            } else {
                kq.segment(mo, m+1) += psfMat.block(mo, no, m+1, n+1) * psf_coeff.segment(no, n+1);
            }
        }
    }
    kq *= 4.0 * afw::geom::PI;

    // [kqb]_{m,n} = \sum_l i^{m-n-l} [kq]_l [tpi]_{l,m,n}
    Eigen::MatrixXd kqb = Eigen::MatrixXd::Zero(result.rows(), result.cols());
    {
        ndarray::Array<double,3,3> b = _tpi.asArray();
        ndarray::Vector<int,3> n, o, x;
        x[0] = 0;
        for (n[1] = o[1] = 0; n[1] <= _rowOrder; o[1] += ++n[1]) {
            for (n[2] = o[2] = 0; n[2] <= _colOrder; o[2] += ++n[2]) {
                for (n[0] = o[0] = bool((n[1]+n[2])%2); n[0] <= psfOrder; (o[0] += ++n[0]) += ++n[0]) {
                    if (n[0] + n[2] < n[1]) continue; // b is triangular
                    double factor = ((n[2] - n[0] - n[1]) % 4) ? -1.0 : 1.0;
                    for (x[1] = 0; x[1] <= n[1]; ++x[1]) {
                        for (x[2] = 0; x[2] <= n[2]; ++x[2]) {
                            // TODO: optimize this inner loop - it's sparse
                            double & out = kqb(o[1] + x[1], o[2] + x[2]);
                            for (x[0] = 0; x[0] <= n[0]; ++x[0]) {
                                out += factor * kq[o[0] + x[0]] * b[o + x];
                            }
                        }
                    }
                }
            }
        }
    }

    result.setZero();
    for (int m = 0, mo = 0; m <= _rowOrder; mo += ++m) {
        for (int n = 0, no = 0; n <= _colOrder; no += ++n) {
            int jo = bool(n % 2);
            for (int j = jo; j <= n; (jo += ++j) += ++j) {
                if ((n - j) % 4) { // (n - j) % 4 is always 0 or 2
                    result.block(mo, no, m+1, n+1)
                        -= kqb.block(mo, jo, m+1, j+1) * modelMat.block(jo, no, j+1, n+1);
                } else {
                    result.block(mo, no, m+1, n+1)
                        += kqb.block(mo, jo, m+1, j+1) * modelMat.block(jo, no, j+1, n+1);
                }
            }
        }
    }

    return _result;
}

int GaussHermiteConvolution::getRowOrder() const { return _impl->getRowOrder(); }

int GaussHermiteConvolution::getColOrder() const { return _impl->getColOrder(); }

ndarray::Array<double const,2,2>
GaussHermiteConvolution::evaluate(
    afw::geom::ellipses::Ellipse & ellipse
) const {
    return _impl->evaluate(ellipse);
}

GaussHermiteConvolution::GaussHermiteConvolution(
    int colOrder,
    ShapeletFunction const & psf
) : _impl(new Impl(colOrder, psf)) {}

GaussHermiteConvolution::~GaussHermiteConvolution() {}

}} // namespace lsst::shapelet