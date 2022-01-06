#pragma once
#include "../Externals/Include/Common.h"

struct Vertex 
{
	glm::vec3 position;
	glm::vec2 texcoord;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
};

struct Texture 
{
	GLuint id;
	aiTextureType type;
	std::string path;
};

class Mesh
{
public:
	void draw(GLuint program) 
    {
		if (vao == 0 || vbo == 0 || ebo == 0) 
			return;
		
		glUseProgram(program);
		glBindVertexArray(this->vao);
		for (std::vector<Texture>::const_iterator it = this->textures.begin(); this->textures.end() != it; it++) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, it->id);
			glUniform1i(glGetUniformLocation(program, "tex"), 0);
		}

		// glDrawArrays(GL_TRIANGLES, 0, this->vertData.size());
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindTexture(GL_TEXTURE_2D, 0); // JJ
		glBindVertexArray(0);
		glUseProgram(0);
	}
	Mesh(): vao(0), vbo(0), ebo(0) { }
	Mesh(const std::vector<Vertex>& vertData, const std::vector<Texture>& textures, const std::vector<GLuint>& indices)
		:vao(0), vbo(0), ebo(0) {
		setup(vertData, textures, indices);
	}
	void setup(const std::vector<Vertex>& vertData, const std::vector<Texture> & textures, const std::vector<GLuint>& indices) {
		this->vertData = vertData;
		this->indices = indices;
		this->textures = textures;
		if (!vertData.empty() && !indices.empty())
			this->setupMesh();
	}
	void final() const {
		glDeleteVertexArrays(1, &this->vao);
		glDeleteBuffers(1, &this->vbo);
		glDeleteBuffers(1, &this->ebo);
	}
	~Mesh() { 
    }
public:
	std::vector<Vertex> vertData;
	std::vector<GLuint> indices;
	std::vector<Texture> textures;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;

	void setupMesh() {
		glGenVertexArrays(1, &this->vao);
		glGenBuffers(1, &this->vbo);
		glGenBuffers(1, &this->ebo);

		glBindVertexArray(this->vao);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * this->vertData.size(), &this->vertData[0], GL_STATIC_DRAW);
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(3 * sizeof(GL_FLOAT)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(5 * sizeof(GL_FLOAT)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(8 * sizeof(GL_FLOAT)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(11 * sizeof(GL_FLOAT)));
		glEnableVertexAttribArray(4);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * this->indices.size(), &this->indices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
};