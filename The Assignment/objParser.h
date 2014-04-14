#ifndef OBJPARSER_H
#define OBJPARSER_H
#include <vector>
#include <GL/glut.h>
class objParser
{
    public:
        static void parse(GLfloat * verts, GLfloat * norms, GLuint * faces);
    private:
        static void calculateFaceNormals(GLfloat * verts, GLfloat * norms, GLuint * faces);
};

#endif // OBJPARSER_H
