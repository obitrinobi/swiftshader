// SwiftShader Software Renderer
//
// Copyright(c) 2005-2013 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

// Program.h: Defines the Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#ifndef LIBGLESV2_PROGRAM_H_
#define LIBGLESV2_PROGRAM_H_

#include "Shader.h"
#include "Context.h"
#include "Shader/PixelShader.hpp"
#include "Shader/VertexShader.hpp"

#include <string>
#include <vector>
#include <set>

namespace es2
{
	class Device;
	class ResourceManager;
	class FragmentShader;
	class VertexShader;

	// Helper struct representing a single shader uniform
	struct Uniform
	{
		Uniform(GLenum type, GLenum precision, const std::string &name, unsigned int arraySize);

		~Uniform();

		bool isArray() const;
		int size() const;
		int registerCount() const;

		const GLenum type;
		const GLenum precision;
		const std::string name;
		const unsigned int arraySize;

		unsigned char *data;
		bool dirty;

		short psRegisterIndex;
		short vsRegisterIndex;
	};

	// Struct used for correlating uniforms/elements of uniform arrays to handles
	struct UniformLocation
	{
		UniformLocation(const std::string &name, unsigned int element, unsigned int index);

		std::string name;
		unsigned int element;
		unsigned int index;
	};

	class Program
	{
	public:
		Program(ResourceManager *manager, GLuint handle);

		~Program();

		bool attachShader(Shader *shader);
		bool detachShader(Shader *shader);
		int getAttachedShadersCount() const;

		sw::PixelShader *getPixelShader();
		sw::VertexShader *getVertexShader();

		void bindAttributeLocation(GLuint index, const char *name);
		GLuint getAttributeLocation(const char *name);
		int getAttributeStream(int attributeIndex);

		GLint getSamplerMapping(sw::SamplerType type, unsigned int samplerIndex);
		TextureType getSamplerTextureType(sw::SamplerType type, unsigned int samplerIndex);

		GLint getUniformLocation(std::string name);
		bool setUniform1fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniform2fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniform3fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniform4fv(GLint location, GLsizei count, const GLfloat *v);
		bool setUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value);
		bool setUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value);
		bool setUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value);
		bool setUniform1iv(GLint location, GLsizei count, const GLint *v);
		bool setUniform2iv(GLint location, GLsizei count, const GLint *v);
		bool setUniform3iv(GLint location, GLsizei count, const GLint *v);
		bool setUniform4iv(GLint location, GLsizei count, const GLint *v);

		bool getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params);
		bool getUniformiv(GLint location, GLsizei *bufSize, GLint *params);

		void dirtyAllUniforms();
		void applyUniforms();

		void link();
		bool isLinked();
		int getInfoLogLength() const;
		void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog);
		void getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders);

		void getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
		GLint getActiveAttributeCount() const;
		GLint getActiveAttributeMaxLength() const;

		void getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
		GLint getActiveUniformCount() const;
		GLint getActiveUniformMaxLength() const;

		void addRef();
		void release();
		unsigned int getRefCount() const;
		void flagForDeletion();
		bool isFlaggedForDeletion() const;

		void validate();
		bool validateSamplers(bool logErrors);
		bool isValidated() const;

		unsigned int getSerial() const;

	private:
		void unlink();

		int packVaryings(const Varying *packing[][4]);
		bool linkVaryings();

		bool linkAttributes();
		int getAttributeBinding(const std::string &name);

		bool linkUniforms(Shader *shader);
		bool defineUniform(GLenum shader, GLenum type, GLenum precision, const std::string &_name, unsigned int arraySize, int registerIndex);
		bool applyUniform1bv(GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform2bv(GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform3bv(GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform4bv(GLint location, GLsizei count, const GLboolean *v);
		bool applyUniform1fv(GLint location, GLsizei count, const GLfloat *v);
		bool applyUniform2fv(GLint location, GLsizei count, const GLfloat *v);
		bool applyUniform3fv(GLint location, GLsizei count, const GLfloat *v);
		bool applyUniform4fv(GLint location, GLsizei count, const GLfloat *v);
		bool applyUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value);
		bool applyUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value);
		bool applyUniform1iv(GLint location, GLsizei count, const GLint *v);
		bool applyUniform2iv(GLint location, GLsizei count, const GLint *v);
		bool applyUniform3iv(GLint location, GLsizei count, const GLint *v);
		bool applyUniform4iv(GLint location, GLsizei count, const GLint *v);    

		void appendToInfoLog(const char *info, ...);
		void resetInfoLog();

		static unsigned int issueSerial();

	private:
		es2::Device *device;
		FragmentShader *fragmentShader;
		VertexShader *vertexShader;

		sw::PixelShader *pixelBinary;
		sw::VertexShader *vertexBinary;
    
		std::set<std::string> attributeBinding[MAX_VERTEX_ATTRIBS];
		sh::Attribute linkedAttribute[MAX_VERTEX_ATTRIBS];
		int attributeStream[MAX_VERTEX_ATTRIBS];

		struct Sampler
		{
			bool active;
			GLint logicalTextureUnit;
			TextureType textureType;
		};

		Sampler samplersPS[MAX_TEXTURE_IMAGE_UNITS];
		Sampler samplersVS[MAX_VERTEX_TEXTURE_IMAGE_UNITS];

		typedef std::vector<Uniform*> UniformArray;
		UniformArray uniforms;
		typedef std::vector<UniformLocation> UniformIndex;
		UniformIndex uniformIndex;

		bool linked;
		bool orphaned;   // Flag to indicate that the program can be deleted when no longer in use
		char *infoLog;
		bool validated;

		unsigned int referenceCount;
		const unsigned int serial;

		static unsigned int currentSerial;

		ResourceManager *resourceManager;
		const GLuint handle;
	};
}

#endif   // LIBGLESV2_PROGRAM_H_
