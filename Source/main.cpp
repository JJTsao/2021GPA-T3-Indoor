#include "../Externals/Include/Common.h"
#include "model.h"
#include "mesh.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define SHADOW_MAP_SIZE 4096
#define PI 3.14159265358979323846f

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

/********** Global Variables Top **********/
mat4 proj_matrix = mat4(1.0f);
mat4 view_matrix = mat4(1.0f);

struct Program
{
	GLuint prog;

	GLuint m_mat;
	GLuint v_mat;
	GLuint p_mat;

	// GLuint fbo;

	GLuint tex_loc;
};

struct Program modelProg, depthProg;
GLuint tex_array[8];

int du = 90, oldmx = -1, oldmy = -1;
double r = 1.5f, h = 0.0f, c = PI / 180.0f;
vec3 eye_pos = vec3(2.66618f, 1.01052f, -2.44763f);
vec3 tar_pos = vec3(0.0f, 0.0f, 0.0f);
vec3 cam_up = vec3(0.0f, 1.0f, 0.0f);

int window_width;
int window_height;

Model model, trice;
GLuint trice_normal_map;
int normal_mapping_flag = 0;

// GLuint depthMap;
/********** Global Variables Bottom **********/

char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char **srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete[] srcp[0];
	delete[] srcp;
}

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// ----- Start Initialize Model Shader Program -----
	modelProg.prog = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	char** vsSource = loadShaderSource("vertex.vs.glsl");
	glShaderSource(vs, 1, vsSource, NULL);
	freeShaderSource(vsSource);
	glCompileShader(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** fsSource = loadShaderSource("fragment.fs.glsl");
	glShaderSource(fs, 1, fsSource, NULL);
	freeShaderSource(fsSource);
	glCompileShader(fs);

	glAttachShader(modelProg.prog, vs);
	glAttachShader(modelProg.prog, fs);
	shaderLog(vs);
	shaderLog(fs);

	glLinkProgram(modelProg.prog);
	glUseProgram(modelProg.prog);

	modelProg.m_mat = glGetUniformLocation(modelProg.prog, "m_mat");
	modelProg.v_mat = glGetUniformLocation(modelProg.prog, "v_mat");
	modelProg.p_mat = glGetUniformLocation(modelProg.prog, "p_mat");
	modelProg.tex_loc = glGetUniformLocation(modelProg.prog, "tex");

	model.loadModel("../Assets/indoor_models_release/Grey_White_Room.obj");
	trice.loadModel("../Assets/indoor_models_release/trice.obj");

	texture_data tdata = loadImg("./indoor_models_release/tricnorm.jpg");

	glGenTextures(1, &trice_normal_map);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, trice_normal_map);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	// cout << trice.meshes.at(0).vertData.at(0).normal.x << endl;
	// ----- End Initialize Model Shader Program -----

	// ----- Start Initialize Depth Shader Program -----
	depthProg.prog = glCreateProgram();
	GLuint shadow_vs = glCreateShader(GL_VERTEX_SHADER);
	char** shadowvsSource = loadShaderSource("depth.vs.glsl");
	glShaderSource(shadow_vs, 1, shadowvsSource, NULL);
	freeShaderSource(shadowvsSource);
	glCompileShader(shadow_vs);

	GLuint shadow_fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** shadowfsSource = loadShaderSource("depth.fs.glsl");
	glShaderSource(shadow_fs, 1, shadowfsSource, NULL);
	freeShaderSource(shadowfsSource);
	glCompileShader(shadow_fs);

	glAttachShader(depthProg.prog, shadow_vs);
	glAttachShader(depthProg.prog, shadow_fs);
	shaderLog(shadow_vs);
	shaderLog(shadow_fs);

	glLinkProgram(depthProg.prog);
	glUseProgram(depthProg.prog);

	depthProg.m_mat = glGetUniformLocation(depthProg.prog, "m_mat");
	depthProg.v_mat = glGetUniformLocation(depthProg.prog, "v_mat");
	depthProg.p_mat = glGetUniformLocation(depthProg.prog, "p_mat");
	// ----- End Initialize Shadow(Depth Shader) Program -----

	// ----- Start Initialize Shadow FBO -----
	//glGenFramebuffers(1, &depthProg.fbo);
	//glBindFramebuffer(GL_FRAMEBUFFER, depthProg.fbo);

	//glGenTextures(1, &depthMap);
	//glBindTexture(GL_TEXTURE_2D, depthMap);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	//glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	for (int i = 0; i < model.meshes.size(); i++) {
		glGenFramebuffers(1, &model.meshes.at(i).fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, model.meshes.at(i).fbo);

		glGenTextures(1, &model.meshes.at(i).shadow_tex);
		glBindTexture(GL_TEXTURE_2D, model.meshes.at(i).shadow_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, model.meshes.at(i).shadow_tex, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			cout << "FBO error" << endl;
	}
	// ----- End Initialize Shadow FBO -----

	printGLError();
}

void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	tar_pos = vec3(cos(c*du), h, sin(c*du));
	view_matrix = lookAt(eye_pos, eye_pos + tar_pos, cam_up);
	mat4 scene_m_matrix = mat4(1.0f);
	mat4 trice_m_matrix = translate(mat4(1.0f), vec3(2.05f, 0.628725f, -1.9f));
	trice_m_matrix = scale(trice_m_matrix, vec3(0.001f));

	/***** Start Generate Shadow Map Phase *****/
	vec3 directional_light_pos = vec3(-2.51449f, 0.477241f, -1.21263f);
	const float shadow_range = 5.0f;
	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 10.0f);
	// mat4 light_view_matrix = lookAt(directional_light_pos, tar_pos, cam_up); 
	// mat4 light_view_matrix = lookAt(directional_light_pos, eye_pos + tar_pos, cam_up);
	mat4 light_view_matrix = lookAt(directional_light_pos, vec3(0.0f), cam_up);

	glEnable(GL_DEPTH_TEST);

	//glBindFramebuffer(GL_FRAMEBUFFER, depthProg.fbo);
	//glClear(GL_DEPTH_BUFFER_BIT);
	//glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glUseProgram(depthProg.prog);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(depthProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
	glUniformMatrix4fv(depthProg.v_mat, 1, GL_FALSE, value_ptr(light_view_matrix));
	glUniformMatrix4fv(depthProg.p_mat, 1, GL_FALSE, value_ptr(light_proj_matrix));
	for (int i = 0; i < model.meshes.size(); i++) {
		Mesh mesh = model.meshes.at(i);
		glBindVertexArray(mesh.vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		glBindFramebuffer(GL_FRAMEBUFFER, model.meshes.at(i).fbo); // 
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
		glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindVertexArray(0);
	}

	glUniformMatrix4fv(depthProg.m_mat, 1, GL_FALSE, value_ptr(trice_m_matrix));
	glUniformMatrix4fv(depthProg.v_mat, 1, GL_FALSE, value_ptr(light_view_matrix));
	glUniformMatrix4fv(depthProg.p_mat, 1, GL_FALSE, value_ptr(light_proj_matrix));
	trice.draw(depthProg.prog);

	glUseProgram(0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	/***** End Generate Shadow Map Phase *****/

	/***** Start Render in Camera View *****/
	mat4 scale_bias_matrix = translate(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));
	scale_bias_matrix = scale(scale_bias_matrix, vec3(0.5f, 0.5f, 0.5f));
	mat4 shadow_sbpv_matrix = scale_bias_matrix * light_proj_matrix * light_view_matrix;

	glViewport(0, 0, window_width, window_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/********** Start Render Scene **********/
	glUseProgram(modelProg.prog);

	/*glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glUniform1i(glGetUniformLocation(modelProg.prog, "shadow_tex"), 2);*/

	glUniformMatrix4fv(modelProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
	glUniformMatrix4fv(modelProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
	glUniformMatrix4fv(modelProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
	glUniformMatrix4fv(glGetUniformLocation(modelProg.prog, "shadow_matrix"), 1, GL_FALSE, value_ptr(shadow_sbpv_matrix * scene_m_matrix));
	glUniform1i(glGetUniformLocation(modelProg.prog, "obj_id"), 0);
	
	for (int i = 0; i < model.meshes.size(); i++) {
		Mesh mesh = model.meshes.at(i);
		glBindVertexArray(mesh.vao);
		glUniform3fv(glGetUniformLocation(modelProg.prog, "Ka"), 1, value_ptr(mesh.mats.Ka));
		glUniform3fv(glGetUniformLocation(modelProg.prog, "Kd"), 1, value_ptr(mesh.mats.Kd));
		glUniform3fv(glGetUniformLocation(modelProg.prog, "Ks"), 1, value_ptr(mesh.mats.Ks));

		if (mesh.textures.size() == 0) {
			glUniform1i(glGetUniformLocation(modelProg.prog, "default_tex"), 1);
		}
		else {
			glUniform1i(glGetUniformLocation(modelProg.prog, "default_tex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mesh.textures.at(0).id);
			glUniform1i(glGetUniformLocation(modelProg.prog, "tex"), 0);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, model.meshes.at(i).shadow_tex);
		glUniform1i(glGetUniformLocation(modelProg.prog, "shadow_tex"), 2);

		glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	}
	glUseProgram(0);
	/********** End Render Scene **********/
	
	/********** Start Render Trice **********/
	glUseProgram(modelProg.prog);
	glUniformMatrix4fv(modelProg.m_mat, 1, GL_FALSE, value_ptr(trice_m_matrix));
	glUniformMatrix4fv(modelProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
	glUniformMatrix4fv(modelProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
	glUniformMatrix4fv(glGetUniformLocation(modelProg.prog, "shadow_matrix"), 1, GL_FALSE, value_ptr(shadow_sbpv_matrix * trice_m_matrix));
	glUniform1i(glGetUniformLocation(modelProg.prog, "obj_id"), 1);
	glUniform1i(glGetUniformLocation(modelProg.prog, "normal_mapping_flag"), normal_mapping_flag);
	Mesh trice_mesh = trice.meshes.at(0);
	glUniform3fv(glGetUniformLocation(modelProg.prog, "Ka"), 1, value_ptr(trice_mesh.mats.Kd)); // Ka = Kd for this project
	glUniform3fv(glGetUniformLocation(modelProg.prog, "Kd"), 1, value_ptr(trice_mesh.mats.Kd));
	glUniform3fv(glGetUniformLocation(modelProg.prog, "Ks"), 1, value_ptr(trice_mesh.mats.Ks));
	

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, trice_normal_map);
	glUniform1i(glGetUniformLocation(modelProg.prog, "trice_normal"), 1);

	trice.draw(modelProg.prog);
	glUseProgram(0);
	/********** End Render Trice **********/

	/***** End Render in Camera View *****/

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	window_width = width;
	window_height = height;
	float viewportAspect = (float)width / (float)height;
	float fov = 80.0f;
	proj_matrix = perspective(radians(fov), viewportAspect, 0.1f, 1000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
		oldmx = x, oldmy = y;
	}
	else if (state == GLUT_UP)
	{
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}

void My_Mouse_Moving(int x, int y)
{
	du -= 0.08f * (x - oldmx);
	h += 0.001f * (y - oldmy);
	if (h > 0.5f) h = 0.5f;
	else if(h < -1.0f) h = -1.0f;
	oldmx = x, oldmy = y;
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	mat4 rot_mat;
	switch (key)
	{
	case 'w':
	case 'W':
		eye_pos += 0.02f * (tar_pos);
		break;
	case 's':
	case 'S':
		eye_pos += 0.02f * (-tar_pos);
		break;
	case 'a':
	case 'A':
		eye_pos += 0.02f * cross(cam_up, tar_pos);
		break;
	case 'd':
	case 'D':
		eye_pos += 0.02f * (-cross(cam_up, tar_pos));
		break;
	case 'z':
	case 'Z':
		eye_pos += 0.1f * (cam_up);
		break;
	case 'x':
	case 'X':
		eye_pos += 0.1f * (-cam_up);
		break;
	case 'n':
	case 'N':
		normal_mapping_flag = !normal_mapping_flag;
		break;
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}


int main(int argc, char *argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1440, 900);
	glutCreateWindow("Final Group 3 Indoor"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_Mouse_Moving);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}