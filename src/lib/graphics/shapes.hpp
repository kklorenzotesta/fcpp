// Copyright © 2021 Giorgio Audrito and Luigi Rapetta. All Rights Reserved.

#ifndef FCPP_GRAPHICS_SHAPES_H_
#define FCPP_GRAPHICS_SHAPES_H_

#include <vector>


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Supported shapes for representing nodes.
enum class shape { cube, tetrahedron, sphere, SIZE };

//! @brief Supported pointers to vertex buffers.
enum class vertex { font, singleLine, star, plane, grid, SIZE };

//! @brief Supported pointers to index buffers.
enum class index { plane, gridNorm, gridHigh, SIZE };


//! @brief Namespace containing objects of internal use.
namespace internal {


//! @brief Collection of vertices.
struct VertexData {
    //! @brief Raw data of triangles as points and normals.
    std::vector<float> data;

    //! @brief Index of the start of data (as points) for all the three colors; size[3] corresponds to the total number of points.
    size_t size[4];

    //! @brief Pointer to the start of data for color `i`.
    inline float const* operator[](size_t i) const {
        return data.data() + size[i] * 6;
    }

    //! @brief Inserts a point in the raw data.
    inline void push_point(float x, float y, float z) {
        data.push_back(x);
        data.push_back(y);
        data.push_back(z);
        data.push_back(0);
        data.push_back(0);
        data.push_back(0);
    }

    //! @brief Inserts a point in the raw data.
    inline void push_point(std::vector<float> const& xs) {
        push_point(xs[0], xs[1], xs[2]);
    }

    //! @brief Compute normals of every triangle.
    void normalize();

    //! @brief Adds symmetric triangles (with respect to the origin).
    void symmetrize();
};

//! @brief Class holding the collections of vertices for every shape.
class Shapes {
public:
    //! @brief Constructor.
    inline Shapes() {
        tetrahedron(m_vertices[(size_t)shape::tetrahedron]);
        cube(m_vertices[(size_t)shape::cube]);
        tetrahedron(m_vertices[(size_t)shape::sphere]);
    }

    //! @brief Const access.
    inline VertexData const& operator[](shape s) const {
        return m_vertices[(size_t)s];
    }

private:
    //! @brief Generates vertex data for a tetrahedron.
    void tetrahedron(VertexData&);

    //! @brief Generates vertex data for a cube.
    void cube(VertexData&);

    //! @brief THe collection of vertices for every shape.
    VertexData m_vertices[(size_t)shape::SIZE];
};


}


}

#endif // FCPP_GRAPHICS_SHAPES_H_
