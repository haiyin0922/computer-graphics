#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 600;

int window_width = 1200;
int window_height = 600;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	LightEditing = 3,
	ShininessEditing = 4
};

GLuint iLocLightIdxV;
GLuint iLocLightIdxP;
GLuint iLocKa;
GLuint iLocKd;
GLuint iLocKs;
GLint iLocMVP;
GLuint iLocNormalMat;
GLuint iLocModelMat;
GLuint iLocViewMat;

struct iLocLightInfo
{
	GLuint position;
	GLuint spotDirect;
	GLuint spotExponent;
	GLuint spotCutoff;
	GLuint Ld;
	GLuint La;
	GLuint Ls;
	GLuint Ac;
	GLuint Al;
	GLuint Aq;
	GLuint shininess;
} iLocLightInfo[3];

struct LightInfo
{
	Vector3 position;
	Vector3 spotDirect;
	Vector3 diffuse;
	Vector3 ambient;
	Vector3 specular;
	float spotExponent;
	float spotCutoff;
	float Ac;
	float Al;
	float Aq;
	float shininess;
} lightInfo[3];

struct PhongMaterial
{
	Vector3 Kd;
	Vector3 Ka;
	Vector3 Ks;
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;

int cur_idx = 0; // represent which model should be rendered now
int light_idx = 0;
int vertex_or_perpixel = 0;

static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{
	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}

// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x) * rotateY(vec.y) * rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	Matrix4 T = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1
	);

	Vector3 Rz = (main_camera.position - main_camera.center).normalize();
	Vector3 Rx = -Rz.cross(main_camera.up_vector).normalize();
	Vector3 Ry = Rz.cross(Rx);

	Matrix4 R = Matrix4(
		Rx.x, Rx.y, Rx.z, 0,
		Ry.x, Ry.y, Ry.z, 0,
		Rz.x, Rz.y, Rz.z, 0,
		0, 0, 0, 1
	);

	view_matrix = R * T;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;

	GLfloat tx = -((proj.right + proj.left) / (proj.right - proj.left));
	GLfloat ty = -((proj.top + proj.bottom) / (proj.top - proj.bottom));
	GLfloat tz = -((proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip));

	project_matrix = Matrix4(
		2 / (proj.right - proj.left), 0, 0, tx,
		0, 2 / (proj.top - proj.bottom), 0, ty,
		0, 0, -2 / (proj.farClip - proj.nearClip), tz,
		0, 0, 0, 1
	);
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;

	GLfloat f = 1 / tan(proj.fovy / (360 / 3.14));

	project_matrix = Matrix4(
		f / proj.aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), (2 * proj.farClip * proj.nearClip) / (proj.nearClip - proj.farClip),
		0, 0, -1, 0
	);
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	// [TODO] change your aspect ratio
	if (cur_proj_mode == Perspective) {
		window_width = width;
		window_height = height;
		proj.aspect = (float)(width / 2) / (float)height;
		setPerspective();
	}
	else {
		setOrthogonal();
	}
}

// Render function for display rendering
void RenderScene(void)
{	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glUniform1i(iLocLightIdxV, light_idx);
	glUniform1i(iLocLightIdxP, light_idx);

	glUniform3f(iLocLightInfo[0].position, lightInfo[0].position.x, lightInfo[0].position.y, lightInfo[0].position.z);
	glUniform3f(iLocLightInfo[0].Ld, lightInfo[0].diffuse.x, lightInfo[0].diffuse.y, lightInfo[0].diffuse.z);
	glUniform3f(iLocLightInfo[0].La, lightInfo[0].ambient.x, lightInfo[0].ambient.y, lightInfo[0].ambient.z);
	glUniform3f(iLocLightInfo[0].Ls, lightInfo[0].specular.x, lightInfo[0].specular.y, lightInfo[0].specular.z);
	glUniform1f(iLocLightInfo[0].shininess, lightInfo[0].shininess);

	glUniform3f(iLocLightInfo[1].position, lightInfo[1].position.x, lightInfo[1].position.y, lightInfo[1].position.z);
	glUniform3f(iLocLightInfo[1].Ld, lightInfo[1].diffuse.x, lightInfo[1].diffuse.y, lightInfo[1].diffuse.z);
	glUniform3f(iLocLightInfo[1].La, lightInfo[1].ambient.x, lightInfo[1].ambient.y, lightInfo[1].ambient.z);
	glUniform3f(iLocLightInfo[1].Ls, lightInfo[1].specular.x, lightInfo[1].specular.y, lightInfo[1].specular.z);
	glUniform1f(iLocLightInfo[1].Ac, lightInfo[1].Ac);
	glUniform1f(iLocLightInfo[1].Al, lightInfo[1].Al);
	glUniform1f(iLocLightInfo[1].Aq, lightInfo[1].Aq);
	glUniform1f(iLocLightInfo[1].shininess, lightInfo[1].shininess);

	glUniform3f(iLocLightInfo[2].position, lightInfo[2].position.x, lightInfo[2].position.y, lightInfo[2].position.z);
	glUniform3f(iLocLightInfo[2].spotDirect, lightInfo[2].spotDirect.x, lightInfo[2].spotDirect.y, lightInfo[2].spotDirect.z);
	glUniform1f(iLocLightInfo[2].spotExponent, lightInfo[2].spotExponent);
	glUniform1f(iLocLightInfo[2].spotCutoff, lightInfo[2].spotCutoff);
	glUniform3f(iLocLightInfo[2].Ld, lightInfo[2].diffuse.x, lightInfo[2].diffuse.y, lightInfo[2].diffuse.z);
	glUniform3f(iLocLightInfo[2].La, lightInfo[2].ambient.x, lightInfo[2].ambient.y, lightInfo[2].ambient.z);
	glUniform3f(iLocLightInfo[2].Ls, lightInfo[2].specular.x, lightInfo[2].specular.y, lightInfo[2].specular.z);
	glUniform1f(iLocLightInfo[2].Ac, lightInfo[2].Ac);
	glUniform1f(iLocLightInfo[2].Al, lightInfo[2].Al);
	glUniform1f(iLocLightInfo[2].Aq, lightInfo[2].Aq);
	glUniform1f(iLocLightInfo[2].shininess, lightInfo[2].shininess);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];
	// [TODO] multiply all the matrix
	MVP = project_matrix * view_matrix * T * R * S;
	// [TODO] row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12];  mvp[7] = MVP[13];   mvp[11] = MVP[14];   mvp[15] = MVP[15];

	Matrix4 Normal = view_matrix * T * R * S;
	Matrix4 Model = T * R * S;

	glUniformMatrix4fv(iLocNormalMat, 1, GL_FALSE, Normal.getTranspose());
	glUniformMatrix4fv(iLocModelMat, 1, GL_FALSE, Model.getTranspose());
	glUniformMatrix4fv(iLocViewMat, 1, GL_FALSE, view_matrix.getTranspose());

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);

	glUniform1i(vertex_or_perpixel, 0);
	glViewport(0, 0, window_width / 2, window_height);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glUniform3f(iLocKa, models[cur_idx].shapes[i].material.Ka.x, models[cur_idx].shapes[i].material.Ka.y, models[cur_idx].shapes[i].material.Ka.z);
		glUniform3f(iLocKd, models[cur_idx].shapes[i].material.Kd.x, models[cur_idx].shapes[i].material.Kd.y, models[cur_idx].shapes[i].material.Kd.z);
		glUniform3f(iLocKs, models[cur_idx].shapes[i].material.Ks.x, models[cur_idx].shapes[i].material.Ks.y , models[cur_idx].shapes[i].material.Ks.z);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}

	glUniform1i(vertex_or_perpixel, 1);
	glViewport(window_width / 2, 0, window_width / 2, window_height);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glUniform3f(iLocKa, models[cur_idx].shapes[i].material.Ka.x, models[cur_idx].shapes[i].material.Ka.y, models[cur_idx].shapes[i].material.Ka.z);
		glUniform3f(iLocKd, models[cur_idx].shapes[i].material.Kd.x, models[cur_idx].shapes[i].material.Kd.y, models[cur_idx].shapes[i].material.Kd.z);
		glUniform3f(iLocKs, models[cur_idx].shapes[i].material.Ks.x, models[cur_idx].shapes[i].material.Ks.y, models[cur_idx].shapes[i].material.Ks.z);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	} 

}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (action) {
		switch (key) {
		case 90:	//Z
			cur_idx = (cur_idx + 5 - 1) % 5;
			break;
		case 88:	//X
			cur_idx = (cur_idx + 1) % 5;
			break;
		case 84:	//T
			cur_trans_mode = GeoTranslation;
			break;
		case 83:	//S
			cur_trans_mode = GeoScaling;
			break;
		case 82:	//R
			cur_trans_mode = GeoRotation;
			break;
		case 76:	//L
			light_idx = (light_idx + 1) % 3;
			break;
		case 75:	//K
			cur_trans_mode = LightEditing;
			break;
		case 74:	//J
			cur_trans_mode = ShininessEditing;
			break;
		default:
			break;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	switch (cur_trans_mode) {
	case GeoTranslation:
		models[cur_idx].position.z += yoffset * 0.1;
		break;
	case GeoRotation:
		models[cur_idx].rotation.z += yoffset * 0.1;
		break;
	case GeoScaling:
		models[cur_idx].scale.z += yoffset * 0.1;
		break;
	case LightEditing:
		if (light_idx == 2) {
			lightInfo[light_idx].spotCutoff += yoffset;
		}
		else {
			lightInfo[light_idx].diffuse += Vector3(yoffset * 0.1, yoffset * 0.1, yoffset * 0.1);
		}
		break;
	case ShininessEditing:
		lightInfo[0].shininess += yoffset;
		lightInfo[1].shininess += yoffset;
		lightInfo[2].shininess += yoffset;
		break;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if (!button && action) {
		mouse_pressed = true;
	}
	else {
		mouse_pressed = false;
	}
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	if (!mouse_pressed) {
		starting_press_x = xpos;
		starting_press_y = ypos;
	}
	if (mouse_pressed) {
		switch (cur_trans_mode)
		{
		case GeoTranslation:
			models[cur_idx].position.x += (xpos - starting_press_x) * (1.0 / 400.0);
			models[cur_idx].position.y -= (ypos - starting_press_y) * (1.0 / 400.0);
			break;
		case GeoRotation:
			models[cur_idx].rotation.x += 3.14 / 180.0 * (ypos - starting_press_y) * (45.0 / 400.0);
			models[cur_idx].rotation.y += 3.14 / 180.0 * (xpos - starting_press_x) * (45.0 / 400.0);
			break;
		case GeoScaling:
			models[cur_idx].scale.x += (xpos - starting_press_x) * (1.0 / 400.0);
			models[cur_idx].scale.y += (ypos - starting_press_y) * (1.0 / 400.0);
			break;
		case LightEditing:
			lightInfo[light_idx].position.x += (xpos - starting_press_x) * (1.0 / 400.0);
			lightInfo[light_idx].position.y -= (ypos - starting_press_y) * (1.0 / 400.0);
			break;
		default:
			break;
		}
		starting_press_x = xpos;
		starting_press_y = ypos;
	}
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");
	
	iLocLightIdxV = glGetUniformLocation(p, "lightIdxV");
	iLocLightIdxP = glGetUniformLocation(p, "lightIdxP");
	iLocKa = glGetUniformLocation(p, "material.Ka");
	iLocKd = glGetUniformLocation(p, "material.Kd");
    iLocKs = glGetUniformLocation(p, "material.Ks");
   

	iLocNormalMat = glGetUniformLocation(p, "NormalMat");
	iLocViewMat = glGetUniformLocation(p, "ViewMat");
	iLocModelMat = glGetUniformLocation(p, "ModelMat");

	iLocLightInfo[0].position = glGetUniformLocation(p, "light[0].position");
	iLocLightInfo[0].La = glGetUniformLocation(p, "light[0].La");
	iLocLightInfo[0].Ld = glGetUniformLocation(p, "light[0].Ld");
	iLocLightInfo[0].Ls = glGetUniformLocation(p, "light[0].Ls");
	iLocLightInfo[0].spotDirect = glGetUniformLocation(p, "light[0].spotDirect");
	iLocLightInfo[0].spotCutoff = glGetUniformLocation(p, "light[0].spotCutoff");
	iLocLightInfo[0].spotExponent = glGetUniformLocation(p, "light[0].spotExponent");
	iLocLightInfo[0].shininess = glGetUniformLocation(p, "light[0].shininess");

	iLocLightInfo[1].position = glGetUniformLocation(p, "light[1].position");
	iLocLightInfo[1].La = glGetUniformLocation(p, "light[1].La");
	iLocLightInfo[1].Ld = glGetUniformLocation(p, "light[1].Ld");
	iLocLightInfo[1].Ls = glGetUniformLocation(p, "light[1].Ls");
	iLocLightInfo[1].Ac = glGetUniformLocation(p, "light[1].Ac");
	iLocLightInfo[1].Al = glGetUniformLocation(p, "light[1].Al");
	iLocLightInfo[1].Aq = glGetUniformLocation(p, "light[1].Aq");
	iLocLightInfo[1].shininess = glGetUniformLocation(p, "light[1].shininess");


	iLocLightInfo[2].position = glGetUniformLocation(p, "light[2].position");
	iLocLightInfo[2].La = glGetUniformLocation(p, "light[2].La");
	iLocLightInfo[2].Ld = glGetUniformLocation(p, "light[2].Ld");
	iLocLightInfo[2].Ls = glGetUniformLocation(p, "light[2].Ls");
	iLocLightInfo[2].spotDirect = glGetUniformLocation(p, "light[2].spotDirect");
	iLocLightInfo[2].spotExponent = glGetUniformLocation(p, "light[2].spotExponent");
	iLocLightInfo[2].spotCutoff = glGetUniformLocation(p, "light[2].spotCutoff");
	iLocLightInfo[2].Ac = glGetUniformLocation(p, "light[2].Ac");
	iLocLightInfo[2].Al = glGetUniformLocation(p, "light[2].Al");
	iLocLightInfo[2].Aq = glGetUniformLocation(p, "light[2].Aq");
	iLocLightInfo[2].shininess = glGetUniformLocation(p, "light[2].shininess");

	vertex_or_perpixel = glGetUniformLocation(p, "vertex_or_perpixel");


	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		
			glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)(WINDOW_WIDTH/2) / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix

	lightInfo[0].position = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[0].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[0].ambient = Vector3(0.15f, 0.15f, 0.15f);
	lightInfo[0].shininess = 64.0f;
	lightInfo[0].specular = Vector3(1.0f, 1.0f, 1.0f);

	lightInfo[1].position = Vector3(0.0f, 2.0f, 1.0f);
	lightInfo[1].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[1].ambient = Vector3(0.15f, 0.15f, 0.15f);
	lightInfo[1].shininess = 64.0f;
	lightInfo[1].specular = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[1].Ac = 0.01;
	lightInfo[1].Al = 0.8;
	lightInfo[1].Aq = 0.1f;


	lightInfo[2].position = Vector3(0.0f, 0.0f, 2.0f);
	lightInfo[2].spotDirect = Vector3(0.0f, 0.0f, -1.0f);
	lightInfo[2].spotExponent = 50;
	lightInfo[2].spotCutoff = 30;
	lightInfo[2].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[2].ambient = Vector3(0.15f, 0.15f, 0.15f);
	lightInfo[2].shininess = 64.0f;
	lightInfo[2].specular = Vector3(1.0f, 1.0f, 1.0f);
	lightInfo[2].Ac = 0.05;
	lightInfo[2].Al = 0.3;
	lightInfo[2].Aq = 0.6f;
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
	LoadModels(model_list[cur_idx]);
	LoadModels(model_list[1]);
	LoadModels(model_list[2]);
	LoadModels(model_list[3]);
	LoadModels(model_list[4]);
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}

int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Student ID HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
