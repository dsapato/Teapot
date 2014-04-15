

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <glm/glm.hpp>
#include "objParser.h"

#define NUM_VERTS 1202
#define NUM_FACES 2256

using namespace std;

void objParser::parse(GLfloat * verts, GLfloat * norms, GLuint * faces){

	string line;
	ifstream myfile ("teapot.txt");
	int vcount = 0;
    int fcount = 0;

	if(myfile.is_open()){
		while(getline(myfile,line) ){
			if(line[0] == 'v'){//Add to vertices
				int spaces = 0;
				string x = "";
				string y = "";
				string z = "";
				for(int i = 2; i < line.size(); i++){
					if(line[i] == ' '){
						spaces++;
					}
					else{
						if(spaces == 0){
							x.push_back(line[i]);
						}
						else if(spaces == 1){
							y.push_back(line[i]);
						}
						else if(spaces == 2){
							z.push_back(line[i]);
						}
					}
				}
				verts[vcount] = (GLfloat)(atof(x.c_str()));
                vcount++;
				verts[vcount] = (GLfloat)(atof(y.c_str()));
				vcount++;
				verts[vcount] = (GLfloat)(atof(z.c_str()));
				vcount++;
                //cout<<verts[vcount - 3]<<", " <<verts[vcount - 2]<<", " <<verts[vcount - 1]<<", " <<endl;
            }
			else if(line[0] == 'f'){//Add to faces
				int spaces = 0;
				string x = "";
				string y = "";
				string z = "";
				for(int i = 3; i < line.size(); i++){
					if(line[i] == ' '){
						spaces++;
					}
					else{
						if(spaces == 0){
							x.push_back(line[i]);
						}
						else if(spaces == 1){
							y.push_back(line[i]);
						}
						else if(spaces == 2){
							z.push_back(line[i]);
						}
					}
				}
				faces[fcount] = (GLuint)(atoi(x.c_str()));
				fcount++;
				faces[fcount] = (GLuint)(atoi(y.c_str()));
				fcount++;
				faces[fcount] = (GLuint)(atoi(z.c_str()));
				fcount++;
				//cout<<faces[fcount - 3]<<", " <<faces[fcount - 2]<<", " <<faces[fcount - 1]<<", " <<endl;
            }
    	}
    	myfile.close();
    	calculateFaceNormals(verts,norms,faces);
	}
	else cout << "Unable to open file" << endl;


    //cout << "vount: " << vcount << ", fcount:" << fcount << endl;

	//cout << "done" << endl;
}

void objParser::calculateFaceNormals(GLfloat * verts, GLfloat * norms, GLuint * faces){
    glm::vec3 * faceNormals = (glm::vec3*)malloc(NUM_VERTS*3*sizeof(glm::vec3));

    for(int i = 0; i < NUM_VERTS*3; i+=3){
        faceNormals[i] = glm::vec3(0.0,0.0,0.0);
    }

    for(int i = 0; i < NUM_FACES*3; i+=3){
        glm::vec3 v1 = glm::vec3(verts[faces[i  ]*3-3],verts[faces[i  ]*3-2],verts[faces[i  ]*3-1]);
        glm::vec3 v2 = glm::vec3(verts[faces[i+1]*3-3],verts[faces[i+1]*3-2],verts[faces[i+1]*3-1]);
        glm::vec3 v3 = glm::vec3(verts[faces[i+2]*3-3],verts[faces[i+2]*3-2],verts[faces[i+2]*3-1]);

        glm::vec3 faceNorm = glm::normalize(glm::cross(v2-v1,v3-v1));

        faceNormals[faces[i  ]*3-3] += faceNorm;
        faceNormals[faces[i+1]*3-3] += faceNorm;
        faceNormals[faces[i+2]*3-3] += faceNorm;
    }

    for(int i = 0; i < NUM_VERTS*3; i+=3){
        glm::vec3 normalized = glm::normalize(faceNormals[i]);
        norms[i ] = normalized.x;
        norms[i+1] = normalized.y;
        norms[i+2] = normalized.z;
    }

    free(faceNormals);
}

