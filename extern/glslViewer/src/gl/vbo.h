#pragma once

#include <vector>

#include "opengl.h"
#include "vertexLayout.h"

#define MAX_INDEX_VALUE 65535

/*
 * Vbo - Drawable collection of geometry contained in a vertex buffer and (optionally) an index buffer
 */

class Vbo {
public:

    Vbo();
    Vbo(VertexLayout* _vertexlayout, GLenum _drawMode = GL_TRIANGLES);
    virtual ~Vbo();

    /*
     * Set Vertex Layout for the Vbo object
     */
    void setVertexLayout(VertexLayout* _vertexLayout);

    /*
     * Set Draw mode for the Vbo object
     */
    void setDrawMode(GLenum _drawMode = GL_TRIANGLES);


    /*
     * Adds a single vertex to the mesh; _vertex must be a pointer to the beginning of a vertex structured
     * according to the VertexLayout associated with this mesh
     */
    void addVertex(GLbyte* _vertex);

    /*
     * Adds _nVertices vertices to the mesh; _vertices must be a pointer to the beginning of a contiguous
     * block of _nVertices vertices structured according to the VertexLayout associated with this mesh
     */
    void addVertices(GLbyte* _vertices, int _nVertices);

    /*
     * Adds a single index to the mesh; indices are unsigned shorts
     */
    void addIndex(GLushort* _index);

    /*
     * Adds _nIndices indices to the mesh; _indices must be a pointer to the beginning of a contiguous
     * block of _nIndices unsigned short indices
     */
    void addIndices(GLushort* _indices, int _nIndices);

    int numIndices() const { return m_indices.size(); };
    int numVertices() const { return m_nVertices; };
    VertexLayout* getVertexLayout() { return m_vertexLayout; };

    /*
     * Copies all added vertices and indices into OpenGL buffer objects; After geometry is uploaded,
     * no more vertices or indices can be added
     */
    void upload();

    /*
     * Renders the geometry in this mesh using the ShaderProgram _shader; if geometry has not already
     * been uploaded it will be uploaded at this point
     */
    void draw(const Shader* _shader);

private:

    VertexLayout* m_vertexLayout;

    std::vector<GLbyte> m_vertexData;
    GLuint  m_glVertexBuffer;
    int     m_nVertices;

    std::vector<GLushort> m_indices;
    GLuint  m_glIndexBuffer;
    int     m_nIndices;

    GLenum  m_drawMode;

    bool    m_isUploaded;
};
