// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "shapes.h"
#include <libcurv/geom/export_frag.h>
#include <libcurv/function.h>
#include <sstream>
#include <iostream>

void
print_shape(curv::geom::Shape_Recognizer& shape)
{
    if (shape.is_2d_) std::cerr << "2D";
    if (shape.is_2d_ && shape.is_3d_) std::cerr << "/";
    if (shape.is_3d_) std::cerr << "3D";
    std::cerr << " shape "
        << (shape.bbox_.xmax - shape.bbox_.xmin) << "×"
        << (shape.bbox_.ymax - shape.bbox_.ymin);
    if (shape.is_3d_)
        std::cerr << "×" << (shape.bbox_.zmax - shape.bbox_.zmin);
    std::cerr << "\n";
}

std::string
shape_to_frag(curv::geom::Shape_Recognizer& shape)
{
    std::stringstream f;
    export_frag(shape, f);
    return f.str();
}
