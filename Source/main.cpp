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

typedef struct _MeshData
{
	vector<float> positions;
	vector<float> normals;
	vector<float> texcoords;
	int vertexCount;
	int material_id;
	GLuint vao;
	GLuint vbo;
	GLuint tex;
} MeshData;

struct Program
{
	GLuint prog;

	GLuint m_mat;
	GLuint v_mat;
	GLuint p_mat;

	GLuint fbo;
	GLuint rbo;

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

GLuint depthMap; // texture
/********** Global Variables Bottom **********/

vector<MeshData> My_LoadModel(const char* modelFile, const char* materialDir, struct Program &prog)
{
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string warn, err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelFile, materialDir);
	cout << "Load Models ! Shapes size " << shapes.size() << " Material size " << materials.size() << endl;
	if (!warn.empty()) { std::cout << warn << endl; }
	if (!err.empty()) { std::cout << err << endl; }
	if (!ret) { exit(1); }

	vector<MeshData> meshes;
	for (int s = 0; s < shapes.size(); ++s) {  // for 'ladybug.obj', there is only one object
		MeshData mesh;
		int index_offset = 0;
		for (int f = 0; f < shapes.at(s).mesh.num_face_vertices.size(); ++f) {
			int fv = shapes.at(s).mesh.num_face_vertices.at(f);
			for (int v = 0; v < fv; ++v) {
				tinyobj::index_t idx = shapes.at(s).mesh.indices.at(index_offset + v);
				if (idx.vertex_index != -1) {
					mesh.positions.push_back(attrib.vertices.at(3 * idx.vertex_index + 0));
					mesh.positions.push_back(attrib.vertices.at(3 * idx.vertex_index + 1));
					mesh.positions.push_back(attrib.vertices.at(3 * idx.vertex_index + 2));
				}
				if (idx.texcoord_index != -1) {
					mesh.texcoords.push_back(attrib.texcoords.at(2 * idx.texcoord_index + 0));
					mesh.texcoords.push_back(attrib.texcoords.at(2 * idx.texcoord_index + 1));
				}
				if (idx.normal_index != -1) {
					mesh.normals.push_back(attrib.normals.at(3 * idx.normal_index + 0));
					mesh.normals.push_back(attrib.normals.at(3 * idx.normal_index + 1));
					mesh.normals.push_back(attrib.normals.at(3 * idx.normal_index + 2));
				}
			}
			index_offset += fv;
		}
		mesh.material_id = shapes.at(s).mesh.material_ids.at(0);
		mesh.vertexCount = shapes.at(s).mesh.indices.size();

		glGenVertexArrays(1, &mesh.vao);
		glBindVertexArray(mesh.vao);

		glGenBuffers(1, &mesh.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

		glBufferData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(float) + mesh.texcoords.size() * sizeof(float) + mesh.normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.positions.size() * sizeof(float), mesh.positions.data());
		glBufferSubData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(float), mesh.texcoords.size() * sizeof(float), mesh.texcoords.data());
		glBufferSubData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(float) + mesh.texcoords.size() * sizeof(float), mesh.normals.size() * sizeof(float), mesh.normals.data());

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh.positions.size() * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh.positions.size() * sizeof(float) + mesh.texcoords.size() * sizeof(float)));
		glEnableVertexAttribArray(2);

		cout << mesh.material_id << "\t: " << shapes.at(s).name << "\t\t: " << materials.at(mesh.material_id).diffuse_texname.c_str() << endl;
		if ((mesh.material_id >= 4 && mesh.material_id <= 7) || mesh.material_id == 14 || mesh.material_id == 16 || mesh.material_id == 17 || mesh.material_id == 21)
		{
			// cout << mesh.material_id << ": " << materials.at(mesh.material_id).diffuse_texname.c_str() << endl;
			texture_data tdata = loadImg(materials.at(mesh.material_id).diffuse_texname.c_str());
			// cout << tdata.width << ", " << tdata.height << endl;

			glGenTextures(1, &mesh.tex);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mesh.tex);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		meshes.push_back(mesh);

		mesh.positions.clear();	mesh.positions.shrink_to_fit();
		mesh.texcoords.clear();	mesh.texcoords.shrink_to_fit();
		mesh.normals.clear();	mesh.normals.shrink_to_fit();
	}

	shapes.clear();		shapes.shrink_to_fit();
	materials.clear();	materials.shrink_to_fit();
	
	return meshes;
}

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
	glGenFramebuffers(1, &depthProg.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, depthProg.fbo);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// ----- End Initialize Shadow FBO -----

	printGLError();
}

void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// view_matrix = lookAt(eye_pos, eye_pos + look_dir, cam_up);
	tar_pos = vec3(cos(c*du), h, sin(c*du));
	view_matrix = lookAt(eye_pos, eye_pos + tar_pos, cam_up);
	mat4 scene_m_matrix = mat4(1.0f);
	mat4 trice_m_matrix = mat4(1.0f);

	///***** Start Generate Shadow Map Phase *****/
	//vec3 directional_light_pos = vec3(-2.51449f, 0.477241f, -1.21263f);
	//const float shadow_range = 15.0f;
	//mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 100.0f);
	//mat4 light_view_matrix = lookAt(directional_light_pos, vec3(0.0f), cam_up);
	//mat4 scale_bias_matrix = translate(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f)) * scale(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));

	//mat4 shadow_sbpv_matrix = scale_bias_matrix * light_proj_matrix * light_view_matrix;

	//glEnable(GL_DEPTH_TEST);

	//glBindFramebuffer(GL_FRAMEBUFFER, depthProg.fbo);
	//glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE); // g8
	//glEnable(GL_POLYGON_OFFSET_FILL);
	//glPolygonOffset(4.0f, 4.0f);
	//glUseProgram(depthProg.prog);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glUniformMatrix4fv(depthProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
	//glUniformMatrix4fv(depthProg.v_mat, 1, GL_FALSE, value_ptr(light_view_matrix));
	//glUniformMatrix4fv(depthProg.p_mat, 1, GL_FALSE, value_ptr(light_proj_matrix));
	//// glUniform1i(glGetUniformLocation(depthProg.prog, "obj_id"), 0);
	//for (int i = 0; i < model.meshes.size(); i++) {
	//	Mesh mesh = model.meshes.at(i);
	//	glBindVertexArray(mesh.vao);
	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
	//	glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
	//	glBindVertexArray(0);
	//}

	//// trice.draw(depthProg.prog);
	//glUseProgram(0);
	//glDisable(GL_POLYGON_OFFSET_FILL);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	///***** End Generate Shadow Map Phase *****/


	
	/***** Start Render Scene *****/
	glUseProgram(modelProg.prog);

	glUniformMatrix4fv(modelProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
	glUniformMatrix4fv(modelProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
	glUniformMatrix4fv(modelProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
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
		glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	}
	glUseProgram(0);
	/***** End Render Scene *****/
	
	/***** Start Render Trice *****/
	glUseProgram(modelProg.prog);
	trice_m_matrix = translate(trice_m_matrix, vec3(2.05f, 0.628725f, -1.9f));
	trice_m_matrix = scale(trice_m_matrix, vec3(0.001f));
	glUniformMatrix4fv(modelProg.m_mat, 1, GL_FALSE, value_ptr(trice_m_matrix));
	glUniformMatrix4fv(modelProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
	glUniformMatrix4fv(modelProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
	Mesh trice_mesh = trice.meshes.at(0);
	glUniform3fv(glGetUniformLocation(modelProg.prog, "Ka"), 1, value_ptr(trice_mesh.mats.Kd)); // Ka = Kd for this project
	glUniform3fv(glGetUniformLocation(modelProg.prog, "Kd"), 1, value_ptr(trice_mesh.mats.Kd));
	glUniform3fv(glGetUniformLocation(modelProg.prog, "Ks"), 1, value_ptr(trice_mesh.mats.Ks));
	glUniform1i(glGetUniformLocation(modelProg.prog, "obj_id"), 1);
	glUniform1i(glGetUniformLocation(modelProg.prog, "normal_mapping_flag"), normal_mapping_flag);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, trice_normal_map);
	glUniform1i(glGetUniformLocation(modelProg.prog, "trice_normal"), 1);

	trice.draw(modelProg.prog);
	glUseProgram(0);
	/***** End Render Trice *****/

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
	glutPostRedisplay();
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
		eye_pos += 0.02f * (tar_pos);
		break;
	//case 'a':
	//case 'A':
	//	rot_mat = rotate(mat4(1.0f), 0.1f, cam_up);
	//	look_dir = vec3(rot_mat * vec4(look_dir, 1.0));
	//	break;
	//case 'd':
	//case 'D':
	//	rot_mat = rotate(mat4(1.0f), -0.1f, cam_up);
	//	look_dir = vec3(rot_mat * vec4(look_dir, 1.0));
	//	break;
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