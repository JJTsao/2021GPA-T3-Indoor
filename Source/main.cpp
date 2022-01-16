#include "../Externals/Include/Common.h"
#include "model.h"
#include "mesh.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_NPR_ENABLE 4
#define MENU_NPR_DISABLE 5
#define MENU_DEFER_ENABLE 6
#define MENU_DEFER_DISABLE 7
#define SHADOW_MAP_SIZE 2048
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

	GLuint fbo;
	GLuint rbo;

	GLuint tex_loc;
};

struct Program modelProg, depthProg, nprProg, linesProg, finalProg, geomProg;
GLuint tex_array[8];

int du = 90, oldmx = -1, oldmy = -1;
double r = 1.5f, h = 0.0f, c = PI / 180.0f;
vec3 eye_pos = vec3(2.66618f, 1.01052f, -2.44763f);
vec3 tar_dir = vec3(0.0f, 0.0f, 0.0f);
vec3 cam_up = vec3(0.0f, 1.0f, 0.0f);

int window_width;
int window_height;

Model model, trice, point_light;
GLuint trice_normal_map;
bool normal_mapping_enabled = false;
bool npr_enabled = false;
bool deferred_enabled = false;

GLuint depthMap, originView, brightView;
GLuint final_vao, final_vbo;
static const GLfloat fb_positions[] =
{
	1.0f, -1.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f
};

struct {
	GLuint fbo;
	GLuint rbo;
	GLuint position_map;
	GLuint normal_map;
	GLuint ambient_map;
	GLuint diffuse_map;
	GLuint specular_map;
	GLuint vao;
} gbuffer;
int deferred_flag = 0;
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

void setupBloomFBO()
{
	glDeleteRenderbuffers(1, &modelProg.rbo);
	glDeleteTextures(1, &originView);
	glDeleteTextures(1, &brightView);

	glGenRenderbuffers(1, &modelProg.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, modelProg.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, window_width, window_height);

	glGenFramebuffers(1, &modelProg.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, modelProg.fbo);

	glGenTextures(1, &originView);
	glBindTexture(GL_TEXTURE_2D, originView);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, originView, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &brightView);
	glBindTexture(GL_TEXTURE_2D, brightView);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, brightView, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, modelProg.rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupGeometryBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);

	glGenTextures(5, &gbuffer.position_map); // op

	glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.position_map, 0);

	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer.normal_map, 0);

	glBindTexture(GL_TEXTURE_2D, gbuffer.ambient_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer.ambient_map, 0);

	glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gbuffer.diffuse_map, 0);

	glBindTexture(GL_TEXTURE_2D, gbuffer.specular_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, gbuffer.specular_map, 0);

	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gbuffer.rbo);
}

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	tar_dir = vec3(cos(c*du), h, sin(c*du));
	view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
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
	point_light.loadModel("../Assets/indoor_models_release/Sphere.obj");

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

	// ----- Start Initialize Geometry Program -----
	geomProg.prog = glCreateProgram();
	GLuint geom_vs = glCreateShader(GL_VERTEX_SHADER);
	char** geomvsSource = loadShaderSource("geometry.vs.glsl");
	glShaderSource(geom_vs, 1, geomvsSource, NULL);
	freeShaderSource(geomvsSource);
	glCompileShader(geom_vs);

	GLuint geom_fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** geomfsSource = loadShaderSource("geometry.fs.glsl");
	glShaderSource(geom_fs, 1, geomfsSource, NULL);
	freeShaderSource(geomfsSource);
	glCompileShader(geom_fs);

	glAttachShader(geomProg.prog, geom_vs);
	glAttachShader(geomProg.prog, geom_fs);
	shaderLog(geom_vs);
	shaderLog(geom_fs);

	glLinkProgram(geomProg.prog);

	glGenFramebuffers(1, &gbuffer.fbo);
	glGenVertexArrays(1, &gbuffer.vao);
	glBindVertexArray(gbuffer.vao);
	// ----- End Initialize Geometry Program -----

	// ----- Start Initialize Final Program -----
	finalProg.prog = glCreateProgram();
	GLuint final_vs = glCreateShader(GL_VERTEX_SHADER);
	char** finalvsSource = loadShaderSource("final.vs.glsl");
	glShaderSource(final_vs, 1, finalvsSource, NULL);
	freeShaderSource(finalvsSource);
	glCompileShader(final_vs);

	GLuint final_fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** finalfsSource = loadShaderSource("final.fs.glsl");
	glShaderSource(final_fs, 1, finalfsSource, NULL);
	freeShaderSource(finalfsSource);
	glCompileShader(final_fs);

	glAttachShader(finalProg.prog, final_vs);
	glAttachShader(finalProg.prog, final_fs);
	shaderLog(final_vs);
	shaderLog(final_fs);

	glLinkProgram(finalProg.prog);
	glUseProgram(finalProg.prog);

	glGenVertexArrays(1, &final_vao);
	glBindVertexArray(final_vao);

	glGenBuffers(1, &final_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, final_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fb_positions), fb_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	// ----- End Initialize Final Program -----

	// ----- Start Initialize Toon(NPR Shader) Program -----
	nprProg.prog = glCreateProgram();
	GLuint npr_vs = glCreateShader(GL_VERTEX_SHADER);
	char** nprvsSource = loadShaderSource("npr.vs.glsl");
	glShaderSource(npr_vs, 1, nprvsSource, NULL);
	freeShaderSource(nprvsSource);
	glCompileShader(npr_vs);

	GLuint npr_fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** nprfsSource = loadShaderSource("npr.fs.glsl");
	glShaderSource(npr_fs, 1, nprfsSource, NULL);
	freeShaderSource(nprfsSource);
	glCompileShader(npr_fs);

	glAttachShader(nprProg.prog, npr_vs);
	glAttachShader(nprProg.prog, npr_fs);
	shaderLog(npr_vs);
	shaderLog(npr_fs);

	glLinkProgram(nprProg.prog);
	glUseProgram(nprProg.prog);

	nprProg.m_mat = glGetUniformLocation(nprProg.prog, "m_mat");
	nprProg.v_mat = glGetUniformLocation(nprProg.prog, "v_mat");
	nprProg.p_mat = glGetUniformLocation(nprProg.prog, "p_mat");
	// ----- End Initialize Toon(NPR Shader) Program -----

	// ----- Start Initialize Lines Shader Program -----
	linesProg.prog = glCreateProgram();
	GLuint lines_vs = glCreateShader(GL_VERTEX_SHADER);
	char** linesvsSource = loadShaderSource("lines.vs.glsl");
	glShaderSource(lines_vs, 1, linesvsSource, NULL);
	freeShaderSource(linesvsSource);
	glCompileShader(lines_vs);

	GLuint lines_fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** linesfsSource = loadShaderSource("lines.fs.glsl");
	glShaderSource(lines_fs, 1, linesfsSource, NULL);
	freeShaderSource(linesfsSource);
	glCompileShader(lines_fs);

	glAttachShader(linesProg.prog, lines_vs);
	glAttachShader(linesProg.prog, lines_fs);
	shaderLog(lines_vs);
	shaderLog(lines_fs);

	glLinkProgram(linesProg.prog);
	glUseProgram(linesProg.prog);

	linesProg.m_mat = glGetUniformLocation(linesProg.prog, "m_mat");
	linesProg.v_mat = glGetUniformLocation(linesProg.prog, "v_mat");
	linesProg.p_mat = glGetUniformLocation(linesProg.prog, "p_mat");
	// ----- End Initialize Lines Shader Program -----

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
	}

	glGenFramebuffers(1, &trice.meshes.at(0).fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, trice.meshes.at(0).fbo);

	glGenTextures(1, &trice.meshes.at(0).shadow_tex);
	glBindTexture(GL_TEXTURE_2D, trice.meshes.at(0).shadow_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, trice.meshes.at(0).shadow_tex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// ----- End Initialize Shadow FBO -----

	// cout << GL_MAX_TEXTURE_UNITS << endl;
	printGLError();
}

void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/***** 0. Start Initialize mvp matrices *****/
	
	mat4 scene_m_matrix = mat4(1.0f);
	mat4 trice_m_matrix = translate(mat4(1.0f), vec3(2.05f, 0.628725f, -1.9f));
	trice_m_matrix = scale(trice_m_matrix, vec3(0.001f));
	mat4 point_light_m_matrix = translate(mat4(1.0f), vec3(1.87659f, 0.4625f, 0.103928f));
	point_light_m_matrix = scale(point_light_m_matrix, vec3(0.15f));
	/***** End Initialize mvp matrices *****/

	/***** 1. Start Generate Shadow Map Phase *****/
	vec3 directional_light_pos = vec3(-2.51449f, 0.477241f, -1.21263f);
	const float shadow_range = 5.0f;
	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 10.0f);
	mat4 light_view_matrix = lookAt(directional_light_pos, vec3(0.0f), cam_up);

	glEnable(GL_DEPTH_TEST);

	//glBindFramebuffer(GL_FRAMEBUFFER, depthProg.fbo);
	//glClear(GL_DEPTH_BUFFER_BIT);
	//glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glUseProgram(depthProg.prog);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(depthProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
	glUniformMatrix4fv(depthProg.v_mat, 1, GL_FALSE, value_ptr(light_view_matrix));
	glUniformMatrix4fv(depthProg.p_mat, 1, GL_FALSE, value_ptr(light_proj_matrix));
	for (int i = 0; i < model.meshes.size(); i++) {
		Mesh mesh = model.meshes.at(i);
		glBindVertexArray(mesh.vao);

		if (mesh.textures.size() == 0) {
			glUniform1i(glGetUniformLocation(depthProg.prog, "default_tex"), 1);
		}
		else {
			glUniform1i(glGetUniformLocation(depthProg.prog, "default_tex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mesh.textures.at(0).id);
			glUniform1i(glGetUniformLocation(depthProg.prog, "diffuse_tex"), 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, model.meshes.at(i).fbo); // 
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

		glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	}

	glUniformMatrix4fv(depthProg.m_mat, 1, GL_FALSE, value_ptr(trice_m_matrix));
	glUniformMatrix4fv(depthProg.v_mat, 1, GL_FALSE, value_ptr(light_view_matrix));
	glUniformMatrix4fv(depthProg.p_mat, 1, GL_FALSE, value_ptr(light_proj_matrix));
	glBindFramebuffer(GL_FRAMEBUFFER, trice.meshes.at(0).fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	trice.draw(depthProg.prog);

	glUseProgram(0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	/***** End Generate Shadow Map Phase *****/

	if(deferred_enabled == false) {
		glBindFramebuffer(GL_FRAMEBUFFER, modelProg.fbo); // Offline Rendering
		static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, draw_buffers);

		/***** 2. Start Render in Camera View *****/
		mat4 scale_bias_matrix = translate(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));
		scale_bias_matrix = scale(scale_bias_matrix, vec3(0.5f, 0.5f, 0.5f));
		mat4 shadow_sbpv_matrix = scale_bias_matrix * light_proj_matrix * light_view_matrix;

		glViewport(0, 0, window_width, window_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/********** 2-1. Start Render Scene **********/
		if (npr_enabled) {
			glUseProgram(linesProg.prog);
			glEnable(GL_CULL_FACE);
			glEnable(GL_LINE_SMOOTH);
			glCullFace(GL_FRONT);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(4);
			glUniformMatrix4fv(linesProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
			glUniformMatrix4fv(linesProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
			glUniformMatrix4fv(linesProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
			for (int i = 0; i < model.meshes.size(); i++) {
				Mesh mesh = model.meshes.at(i);
				glBindVertexArray(mesh.vao);
				glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);
			}
			glUniformMatrix4fv(linesProg.m_mat, 1, GL_FALSE, value_ptr(trice_m_matrix));
			glUniformMatrix4fv(linesProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
			glUniformMatrix4fv(linesProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
			trice.draw(linesProg.prog);
			glDisable(GL_CULL_FACE);
			glDisable(GL_LINE_SMOOTH);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glUseProgram(0);

			glUseProgram(nprProg.prog);

			glUniformMatrix4fv(nprProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
			glUniformMatrix4fv(nprProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
			glUniformMatrix4fv(nprProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
			for (int i = 0; i < model.meshes.size(); i++) {
				Mesh mesh = model.meshes.at(i);
				glBindVertexArray(mesh.vao);
				glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);
			}
			glUniformMatrix4fv(nprProg.m_mat, 1, GL_FALSE, value_ptr(trice_m_matrix));
			glUniformMatrix4fv(nprProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
			glUniformMatrix4fv(nprProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
			trice.draw(nprProg.prog);
			glUseProgram(0);
		}
		else {
			glUseProgram(modelProg.prog);

			//glActiveTexture(GL_TEXTURE2);
			//glBindTexture(GL_TEXTURE_2D, depthMap);
			//glUniform1i(glGetUniformLocation(modelProg.prog, "shadow_tex"), 2);

			glUniformMatrix4fv(modelProg.m_mat, 1, GL_FALSE, value_ptr(scene_m_matrix));
			glUniformMatrix4fv(modelProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
			glUniformMatrix4fv(modelProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
			glUniformMatrix4fv(glGetUniformLocation(modelProg.prog, "shadow_matrix"), 1, GL_FALSE, value_ptr(shadow_sbpv_matrix * scene_m_matrix));
			glUniform1i(glGetUniformLocation(modelProg.prog, "obj_id"), 0);

			for (int i = 0; i < model.meshes.size(); i++) {
				Mesh mesh = model.meshes.at(i);
				glBindVertexArray(mesh.vao);
				glUniform3fv(glGetUniformLocation(modelProg.prog, "Ka"), 1, value_ptr(mesh.mats.Kd)); // Ka = Kd
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

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, model.meshes.at(i).shadow_tex);
				glUniform1i(glGetUniformLocation(modelProg.prog, "shadow_tex"), 2);

				glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);

				glBindTexture(GL_TEXTURE_2D, 0);
				glBindVertexArray(0);
			}
			glUseProgram(0);
			/********** End Render Scene **********/

			/********** 2-2. Start Render Trice **********/
			glUseProgram(modelProg.prog);
			glUniformMatrix4fv(modelProg.m_mat, 1, GL_FALSE, value_ptr(trice_m_matrix));
			glUniformMatrix4fv(modelProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
			glUniformMatrix4fv(modelProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
			glUniformMatrix4fv(glGetUniformLocation(modelProg.prog, "shadow_matrix"), 1, GL_FALSE, value_ptr(shadow_sbpv_matrix * trice_m_matrix));
			glUniform1i(glGetUniformLocation(modelProg.prog, "obj_id"), 1);
			glUniform1i(glGetUniformLocation(modelProg.prog, "normal_mapping_flag"), normal_mapping_enabled);
			Mesh trice_mesh = trice.meshes.at(0);
			glUniform3fv(glGetUniformLocation(modelProg.prog, "Ka"), 1, value_ptr(trice_mesh.mats.Kd)); // Ka = Kd for this project
			glUniform3fv(glGetUniformLocation(modelProg.prog, "Kd"), 1, value_ptr(trice_mesh.mats.Kd));
			glUniform3fv(glGetUniformLocation(modelProg.prog, "Ks"), 1, value_ptr(trice_mesh.mats.Ks));

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, trice.meshes.at(0).shadow_tex);
			glUniform1i(glGetUniformLocation(modelProg.prog, "shadow_tex"), 2);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, trice_normal_map);
			glUniform1i(glGetUniformLocation(modelProg.prog, "trice_normal"), 1);

			trice.draw(modelProg.prog);
			glUseProgram(0);
			/********** End Render Trice **********/

			/********** 2-3. Start Render Point Light Sphere **********/
			glUseProgram(modelProg.prog);
			glUniformMatrix4fv(modelProg.m_mat, 1, GL_FALSE, value_ptr(point_light_m_matrix));
			glUniformMatrix4fv(modelProg.v_mat, 1, GL_FALSE, value_ptr(view_matrix));
			glUniformMatrix4fv(modelProg.p_mat, 1, GL_FALSE, value_ptr(proj_matrix));
			glUniform1i(glGetUniformLocation(modelProg.prog, "obj_id"), 2);

			point_light.draw(modelProg.prog);
			glUseProgram(0);
			/********** End Render Point Light Sphere **********/
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		/***** End Render in Camera View *****/

		/***** 3. Start Render in Screen View *****/
		glViewport(0, 0, window_width, window_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(finalProg.prog);
		glBindVertexArray(final_vao);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, originView);
		glUniform1i(glGetUniformLocation(finalProg.prog, "originView"), 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, brightView);
		glUniform1i(glGetUniformLocation(finalProg.prog, "brightView"), 4);
		glUniform1i(glGetUniformLocation(finalProg.prog, "deferred_enabled"), deferred_enabled);
		// glUniform1i(glGetUniformLocation(finalProg.prog, "window_width"), window_width);
		// glUniform1i(glGetUniformLocation(finalProg.prog, "window_height"), window_height);

		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glBindVertexArray(0);
		glUseProgram(0);
		/***** End Render in Screen View *****/
	}

	/***** 4. Start Deferred Shading *****/
	else if (deferred_enabled == true) {
		/********** 4-1. Start Geometry Shader Phase **********/
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
		const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
		glDrawBuffers(5, draw_buffers);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		/*************** 4-1-1. Start Render Scene ***************/
		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, window_width, window_height);
		glUseProgram(geomProg.prog);

		glUniformMatrix4fv(glGetUniformLocation(geomProg.prog, "m_mat"), 1, GL_FALSE, value_ptr(scene_m_matrix));
		glUniformMatrix4fv(glGetUniformLocation(geomProg.prog, "vp_mat"), 1, GL_FALSE, value_ptr(proj_matrix * view_matrix));

		for (int i = 0; i < model.meshes.size(); i++) {
			Mesh mesh = model.meshes.at(i);
			glBindVertexArray(mesh.vao);
			glUniform3fv(glGetUniformLocation(geomProg.prog, "Ka"), 1, value_ptr(mesh.mats.Ka));
			glUniform3fv(glGetUniformLocation(geomProg.prog, "Kd"), 1, value_ptr(mesh.mats.Kd));
			glUniform3fv(glGetUniformLocation(geomProg.prog, "Ks"), 1, value_ptr(mesh.mats.Ks));

			if (mesh.textures.size() == 0) {
				glUniform1i(glGetUniformLocation(geomProg.prog, "default_tex"), 1);
			}
			else {
				glUniform1i(glGetUniformLocation(geomProg.prog, "default_tex"), 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mesh.textures.at(0).id);
				glUniform1i(glGetUniformLocation(geomProg.prog, "tex"), 0);
			}
			glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);
		}
		glUseProgram(0);
		/*************** End Render Scene ***************/

		/*************** 4-1-2. Start Render Trice ***************/
		glUseProgram(geomProg.prog);
		glUniformMatrix4fv(glGetUniformLocation(geomProg.prog, "m_mat"), 1, GL_FALSE, value_ptr(trice_m_matrix));
		glUniformMatrix4fv(glGetUniformLocation(geomProg.prog, "vp_mat"), 1, GL_FALSE, value_ptr(proj_matrix * view_matrix));
		Mesh trice_mesh = trice.meshes.at(0);
		glUniform3fv(glGetUniformLocation(geomProg.prog, "Ka"), 1, value_ptr(trice_mesh.mats.Ka));
		glUniform3fv(glGetUniformLocation(geomProg.prog, "Kd"), 1, value_ptr(trice_mesh.mats.Kd));
		glUniform3fv(glGetUniformLocation(geomProg.prog, "Ks"), 1, value_ptr(trice_mesh.mats.Ks));
		glUniform1i(glGetUniformLocation(geomProg.prog, "default_tex"), 1);

		trice_mesh.draw(geomProg.prog);
		glUseProgram(0);
		/*************** End Render Trice ***************/

		/********** 4-2. Start Deferred Shader Phase **********/
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glUseProgram(finalProg.prog);
		glBindVertexArray(final_vao);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
		glUniform1i(glGetUniformLocation(finalProg.prog, "position_map"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
		glUniform1i(glGetUniformLocation(finalProg.prog, "normal_map"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gbuffer.ambient_map);
		glUniform1i(glGetUniformLocation(finalProg.prog, "ambient_map"), 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
		glUniform1i(glGetUniformLocation(finalProg.prog, "diffuse_map"), 3);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gbuffer.specular_map);
		glUniform1i(glGetUniformLocation(finalProg.prog, "specular_map"), 4);

		glUniform1i(glGetUniformLocation(finalProg.prog, "deferred_enabled"), deferred_enabled);
		glUniform1i(glGetUniformLocation(finalProg.prog, "deferred_flag"), deferred_flag);

		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);
		/********** End Deferred Shader Phase **********/
	}
	/***** End Deferred Shading *****/

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

	glDeleteRenderbuffers(1, &gbuffer.rbo);
	glDeleteTextures(5, &gbuffer.position_map);

	glGenRenderbuffers(1, &gbuffer.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gbuffer.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);

	setupGeometryBuffer();
	//glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	//glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	//glBindTexture(GL_TEXTURE_2D, gbuffer.ambient_map);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	//glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	//glBindTexture(GL_TEXTURE_2D, gbuffer.specular_map);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	/***** Start Model Shader FBO & RBO *****/
	setupBloomFBO();
	/***** End Model Shader FBO & RBO *****/
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
	tar_dir = vec3(cos(c*du), h, sin(c*du));
	view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	mat4 rot_mat;
	switch (key)
	{
	case 'w':
	case 'W':
		eye_pos += 0.02f * (tar_dir);
		view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
		break;
	case 's':
	case 'S':
		eye_pos += 0.02f * (-tar_dir);
		view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
		break;
	case 'a':
	case 'A':
		eye_pos += 0.02f * cross(cam_up, tar_dir);
		view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
		break;
	case 'd':
	case 'D':
		eye_pos += 0.02f * (-cross(cam_up, tar_dir));
		view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
		break;
	case 'z':
	case 'Z':
		eye_pos += 0.1f * (cam_up);
		view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
		break;
	case 'x':
	case 'X':
		eye_pos += 0.1f * (-cam_up);
		view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
		break;
	case 'n':
	case 'N':
		normal_mapping_enabled = !normal_mapping_enabled;
		break;
	case 'e':
	case 'E':
		cout << "New eye_pos: " << endl;
		float x1, y1, z1; cin >> x1 >> y1 >> z1;
		eye_pos = vec3(x1, y1, z1);
		view_matrix = lookAt(eye_pos, eye_pos + tar_dir, cam_up);
		break;
	case 'l':
	case 'L':
		cout << "New look-At: " << endl;
		float x2, y2, z2; cin >> x2 >> y2 >> z2;
		vec3 lookAtCenter = vec3(x2, y2, z2);
		view_matrix = lookAt(eye_pos, lookAtCenter, cam_up);
		break;
	case 'g':
	case 'G':
		deferred_flag = (deferred_flag + 1) % 5;
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
	case MENU_NPR_ENABLE:
		npr_enabled = true;
		break;
	case MENU_NPR_DISABLE:
		npr_enabled = false;
		break;
	case MENU_DEFER_ENABLE:
		deferred_enabled = true;
		break;
	case MENU_DEFER_DISABLE:
		deferred_enabled = false;
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
	int menu_npr = glutCreateMenu(My_Menu);
	int menu_gbuffer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddSubMenu("NPR", menu_npr);
	glutAddSubMenu("G-Buffer", menu_gbuffer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_npr);
	glutAddMenuEntry("Enable", MENU_NPR_ENABLE);
	glutAddMenuEntry("Disable", MENU_NPR_DISABLE);

	glutSetMenu(menu_gbuffer);
	glutAddMenuEntry("Enable", MENU_DEFER_ENABLE);
	glutAddMenuEntry("Disable", MENU_DEFER_DISABLE);

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