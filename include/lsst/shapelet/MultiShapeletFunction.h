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
 
#ifndef LSST_AFW_MATH_SHAPELETS_MULTISHAPELETFUNCTION_H
#define LSST_AFW_MATH_SHAPELETS_MULTISHAPELETFUNCTION_H

#include "lsst/shapelet/ShapeletFunction.h"
#include <list>

namespace lsst { namespace shapelet {

class MultiShapeletFunctionEvaluator;

/**
 *  @brief A multi-scale shapelet function.
 */
class MultiShapeletFunction {
public:

    typedef boost::shared_ptr<MultiShapeletFunction> Ptr;
    typedef boost::shared_ptr<MultiShapeletFunction const> ConstPtr;

    typedef MultiShapeletFunctionEvaluator Evaluator;

    typedef ShapeletFunction Element;

    typedef std::list<Element> ElementList;

    ElementList & getElements() { return _elements; }

    ElementList const & getElements() const { return _elements; }

    /// @brief Normalize the integral of the shapelet function to 1.
    void normalize();

    /// @brief Shift the shapelet function by shifting the ellipse of each element.
    void shiftInPlace(afw::geom::Extent2D const & offset);

    /// @brief Transform the shapelet function by transforming the ellipse of each elements.
    void transformInPlace(afw::geom::AffineTransform const & transform);

    /// @brief Convolve the multi-shapelet function.
    MultiShapeletFunction convolve(ShapeletFunction const & other) const;

    /// @brief Convolve the multi-shapelet function.
    MultiShapeletFunction convolve(MultiShapeletFunction const & other) const;

    /// @brief Construct a helper object that can efficiently evaluate the function.
    Evaluator evaluate() const;

    MultiShapeletFunction() : _elements() {}

    explicit MultiShapeletFunction(ElementList const & elements) : _elements(elements) {}

    explicit MultiShapeletFunction(ShapeletFunction const & element) : _elements(1, element) {}

private:
    ElementList _elements;
};

/**
 *  @brief Evaluates a MultiShapeletFunction.
 *
 *  This is distinct from MultiShapeletFunction to allow the evaluator to construct temporaries
 *  and allocate workspace that will be reused when evaluating at multiple points.
 *
 *  A MultiShapeletFunctionEvaluator is invalidated whenever the MultiShapeletFunction it
 *  was constructed from is modified.
 */
class MultiShapeletFunctionEvaluator {
public:

    typedef boost::shared_ptr<MultiShapeletFunctionEvaluator> Ptr;
    typedef boost::shared_ptr<MultiShapeletFunctionEvaluator const> ConstPtr;

    /// @brief Evaluate at the given point.
    double operator()(double x, double y) const {
        return this->operator()(afw::geom::Point2D(x, y));
    }

    /// @brief Evaluate at the given point.
    double operator()(afw::geom::Point2D const & point) const;

    /// @brief Evaluate at the given point.
    double operator()(afw::geom::Extent2D const & point) const;

    /// @brief Add the function to the given image-like array.
    void addToImage(
        ndarray::Array<double,2,1> const & array,
        afw::geom::Point2I const & xy0 = afw::geom::Point2I()
    ) const;

    /// @brief Evaluate the function on the given image.
    void addToImage(afw::image::Image<double> & image) const {
        addToImage(image.getArray(), image.getXY0());
    }

    /// @brief Compute the definite integral or integral moments.
    double integrate() const;

    /// @brief Return the unweighted dipole and quadrupole moments of the function as an ellipse.
    afw::geom::ellipses::Ellipse computeMoments() const;

    /// @brief Update the evaluator from the given function.
    void update(MultiShapeletFunction const & function);

    /// @brief Construct an evaluator for the given function.
    explicit MultiShapeletFunctionEvaluator(MultiShapeletFunction const & function);

private:
    typedef ShapeletFunctionEvaluator Element;
    typedef std::list<Element> ElementList;
    ElementList _elements;
};

inline MultiShapeletFunctionEvaluator MultiShapeletFunction::evaluate() const {
    return MultiShapeletFunctionEvaluator(*this);
}

}} // namespace lsst::shapelet

#endif // !defined(LSST_AFW_MATH_SHAPELETS_MULTISHAPELETFUNCTION_H)
