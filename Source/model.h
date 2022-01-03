#pragma once
#include "../Externals/Include/Common.h"
#include "../Externals/Include/assimp/Importer.hpp"
#include "mesh.h"

class Model {
public:
	void draw(GLuint program) {
		for (std::vector<Mesh>::iterator it = this->meshes.begin(); this->meshes.end() != it; it++) 
			it->draw(program);
	}
	bool loadModel(std::string filepath) {
		Assimp::Importer importer;

		// const aiScene *secne_obj_ptr = aiImportFile(filepath.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
		const aiScene* secne_obj_ptr = importer.ReadFile(filepath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_CalcTangentSpace);
		if (!secne_obj_ptr || secne_obj_ptr->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !secne_obj_ptr->mRootNode)
			return false;

		this->model_file_dir = filepath.substr(0, filepath.find_last_of('/'));
		if (!this->processNode(secne_obj_ptr->mRootNode, secne_obj_ptr)) 
			return false;

		return true;
	}

	~Model() {
		for (std::vector<Mesh>::const_iterator it = this->meshes.begin(); this->meshes.end() != it; ++it) {
			it->final();
		}
	}
private:
	bool processNode(const aiNode* node, const aiScene* scene_obj_ptr) {
		if (!node || !scene_obj_ptr)
			return false;

		for (size_t i = 0; i < node->mNumMeshes; i++) {
			const aiMesh* mesh_ptr = scene_obj_ptr->mMeshes[node->mMeshes[i]];
			if (mesh_ptr) {
				Mesh mesh_obj;
				if (this->processMesh(mesh_ptr, scene_obj_ptr, mesh_obj))
					this->meshes.push_back(mesh_obj);
			}
		}

		for (size_t i = 0; i < node->mNumChildren; ++i) 
			this->processNode(node->mChildren[i], scene_obj_ptr);

		return true;
	}
	bool processMesh(const aiMesh* mesh_ptr, const aiScene* sceneObjPtr, Mesh& meshObj) {
		if (!mesh_ptr || !sceneObjPtr)
			return false;
		
		std::vector<Vertex> vertData;
		std::vector<Texture> textures;
		std::vector<GLuint> indices;
		
		for (size_t i = 0; i < mesh_ptr->mNumVertices; i++) {
			Vertex vertex;
			
			if (mesh_ptr->HasPositions()) {
				vertex.position.x = mesh_ptr->mVertices[i].x;
				vertex.position.y = mesh_ptr->mVertices[i].y;
				vertex.position.z = mesh_ptr->mVertices[i].z;
			}
			
			if (mesh_ptr->HasTextureCoords(0)) {
				vertex.texcoord.x = mesh_ptr->mTextureCoords[0][i].x;
				vertex.texcoord.y = mesh_ptr->mTextureCoords[0][i].y;
			} else {
				vertex.texcoord = glm::vec2(0.0f, 0.0f);
			}
			
			if (mesh_ptr->HasNormals()) {
				vertex.normal.x = mesh_ptr->mNormals[i].x;
				vertex.normal.y = mesh_ptr->mNormals[i].y;
				vertex.normal.z = mesh_ptr->mNormals[i].z;
			} 

			vertData.push_back(vertex);
		}

		for (size_t i = 0; i < mesh_ptr->mNumFaces; ++i) {
			aiFace face = mesh_ptr->mFaces[i];
			if (face.mNumIndices != 3) {
				return false;
			}
			for (size_t j = 0; j < face.mNumIndices; ++j)
				indices.push_back(face.mIndices[j]);
		
		}

		if (mesh_ptr->mMaterialIndex >= 0) {
			const aiMaterial* materialPtr = sceneObjPtr->mMaterials[mesh_ptr->mMaterialIndex];
			
			std::vector<Texture> diffuseTexture;
			this->processMaterial(materialPtr, sceneObjPtr, aiTextureType_DIFFUSE, diffuseTexture);
			textures.insert(textures.end(), diffuseTexture.begin(), diffuseTexture.end());
		}
		meshObj.setup(vertData, textures, indices);
		return true;
	}
	
	bool processMaterial(const aiMaterial* matPtr, const aiScene* sceneObjPtr,
		const aiTextureType textureType, std::vector<Texture>& textures) {
		textures.clear();

		if (!matPtr || !sceneObjPtr)
			return false;
		
		if (matPtr->GetTextureCount(textureType) <= 0)
			return true;
		
		for (size_t i = 0; i < matPtr->GetTextureCount(textureType); i++) {
			Texture text;
			aiString textPath;
			aiReturn retStatus = matPtr->GetTexture(textureType, i, &textPath);
			if (retStatus != aiReturn_SUCCESS || textPath.length == 0) 
				continue;
			
			std::string image_path = this->model_file_dir + "/" + textPath.C_Str();
			auto it = this->loadedTextureMap.find(image_path);
			if(it == this->loadedTextureMap.end()) {
				// Loading Texture
				texture_data tdata = loadImg(image_path.c_str());

				std::cout << "[Loading \"" << image_path << "\" Texture] width: " << tdata.width << ", height: " << tdata.height << std::endl;

				// Generate Texture 
				glGenTextures(1, &text.id);
				glBindTexture(GL_TEXTURE_2D, text.id);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				text.path = image_path;
				text.type = textureType;
				textures.push_back(text);
				loadedTextureMap[image_path] = text;
				
				delete tdata.data;
			}
			else
				textures.push_back(it->second);
		}
		return true;
	}
private:
	std::vector<Mesh> meshes; 
	std::string model_file_dir;
	std::map<std::string, Texture> loadedTextureMap;
};