/*char *loadshader(char *filename)//From glsl-teapot on class website
{
	std::string strbuf;
	std::string line;
	std::ifstream in(filename);
	while(std::getline(in,line))
		strbuf += line + '\n';

	char *buf = (char *)malloc(strbuf.size()*sizeof(char));
	strcpy(buf,strbuf.c_str());

	return buf;
}

GLuint programObj = 0;

void main{
 	//Initialize shaders and program object
	GLchar *vertexShaderCodeStr = loadshader("basic.vert");
	const GLchar **vertexShaderCode = (const GLchar **)&vertexShaderCodeStr;

	GLchar *fragmentShaderCodeStr = loadshader("basic.frag");
	const GLchar **fragmentShaderCode = (const GLchar **)&fragmentShaderCodeStr;

	int status;
	GLint infologLength = 0;
	GLint charsWritten = 0;
	GLchar infoLog[10000];

	GLuint vertexShaderObj = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderObj, 1, vertexShaderCode, 0);
	glCompileShader(vertexShaderObj); /* Converts to GPU code */
/*
	GLuint fragmentShaderObj = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderObj, 1, fragmentShaderCode, 0);
	glCompileShader(fragmentShaderObj); /* Converts to GPU code */
/*
//	GLuint
	programObj = glCreateProgram();
	glAttachObjectARB(programObj,vertexShaderObj);
	glAttachObjectARB(programObj,fragmentShaderObj);
	glLinkProgram(programObj);	/* Connects shaders & variables *//*
	glUseProgram(programObj); /* OpenGL now uses the shader *//*

	glGetProgramivARB(programObj, GL_LINK_STATUS, &status);

	glGetObjectParameterivARB(programObj,GL_OBJECT_INFO_LOG_LENGTH_ARB,&infologLength);

	if(infologLength > 0) {
		glGetInfoLogARB(programObj,infologLength,&charsWritten,infoLog);
		if(charsWritten)
			printf("InfoLog:\n%s\n\n",infoLog);
	}

}*/
