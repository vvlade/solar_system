#ifndef SOLAR_SYSTEM_MESH_H
#define SOLAR_SYSTEM_MESH_H

#include <vector>
#include <string>
#include <Error.h>
#include "glm/glm.hpp"
#include "glad/glad.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Texture {
    unsigned int id;
    std::string type; // texture_diffuse, texture_specular, texture_normal, texture_height
    std::string path;
};


class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    unsigned int vao;
    std::string glslIdentifierPrefix;

    Mesh(const std::vector<Vertex>& vs, const std::vector<unsigned int>& is,
         const std::vector<Texture>& ts)
         : vertices(vs), indices(is), textures(ts){
        setupMesh();
    }

    void Draw(Shader& shader) {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        for(unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string name = textures[i].type;
            std::string number;

            if(name == "texture_diffuse") {
                number = std::to_string(diffuseNr++);
            } else if(name == "texture_specular") {
                number = std::to_string(specularNr++);
            } else if(name == "texture_normal") {
                number = std::to_string(normalNr++);
            } else if(name == "texture_height") {
                number = std::to_string(heightNr++);
            } else {
                ASSERT(false, "Unknown texture type");
            }

            glUniform1i(glGetUniformLocation(shader.ID, (glslIdentifierPrefix + name + number).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // deactivating all the objects we used
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
private:
    unsigned vbo, ebo;

    void setupMesh() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

        // positions
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Position)));
        glEnableVertexAttribArray(0);

        // normals
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Normal)));
        glEnableVertexAttribArray(1);

        // tex coordinates
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, TexCoords)));
        glEnableVertexAttribArray(2);

        // tangents
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Tangent)));
        glEnableVertexAttribArray(3);

        // bitangents
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, Bitangent)));
        glEnableVertexAttribArray(4);

        glBindVertexArray(0);
    }

};


#endif //SOLAR_SYSTEM_MESH_H
