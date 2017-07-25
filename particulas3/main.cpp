#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "particle.h"
#include "screenCap.h"

#include <ppl.h>
#include <ctime>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.5f, 0.2f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//simulation thingies
bool simulate = false; // esse define se a simulacao esta rodando
bool parallel = true; // esse define se vai usar threads ou nao

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

screenCap cappie;

int main()
{

	std::time_t t1 = time(nullptr);

	srand(static_cast <unsigned> (time(0)));

	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "boing", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader lightShader("lightcaster.vs", "lightcaster.fs");

	// load models
	// -----------
	Model wall_m("C:/Users/dquei/Desktop/part/partdumb8v.obj");
	Model fluid_m("C:/Users/dquei/Desktop/part/part16.obj");

	std::vector<Particle> free;	// vetor que guarda todas as minhas particulas
	float randRange = 0.5f;	// valor maximo que x, y e z podem assumir, para a posição das partículas
	float scaling_factor = 0.05f; // usado para definir o tamanho das particulas
	// esses proximos 3 fatores definem o tamanho da caixa que segura as particulas
	float x_bound = 0.5f;
	float y_bound = 0.5f;
	float z_bound = 0.5f;
	int n_part = 1000; // numero de particulas na simulaçao
	//	inicializando as particulas em posicoes aleatorias, para evitar um equilibrio estranho
	for (int i = 0; i < n_part; i++) {
		Particle p;
		float rx = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / randRange));
		p.pos.x = rx;
		float ry = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / randRange));
		p.pos.y = ry;
		float rz = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / randRange));
		p.pos.z = rz;
		free.push_back(p);
	}

	std::vector<Particle> aux; // esse vetor vai ser utilizado para fazer os calculos a cada frame
	aux = free;
	std::cout << "terminou de settar as posies" << std::endl;
	std::cout << "n de particulas: " << free.size() << std::endl;
	std::cout << "segundos: " << t1 << std::endl;
	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//shader configuration
	lightShader.use();
	lightShader.setInt("material.diffuse", 0);
	lightShader.setInt("material.specular", 0);

	int curFrame = 0;

	// render loop
	// -----------
	//double simulationtime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{

		// esse bloco serve pra definir a quantidade de frames que quero simular
		curFrame++;
		if (curFrame == 1001) {
			break;
		}
		std::cout << "curFrame: " << curFrame << std::endl;

		//particle logic
		// primeiro limpar todas as densidades
		// recalcular a densidade para o passo
		// definir vetores de pressao, viscosidade e com isso aceleracao, velocidade e por fim nova posicao
		// tratar colisao com a caixa
		if (simulate) {
			// paralelo
			if (parallel) {
				int size = free.size();
				concurrency::parallel_for(0, size, [&](int i) {
					aux[i].clean_density();
				});


				concurrency::parallel_for(0, size, [&](int i) {
					aux[i].def_density(free);
				});


				concurrency::parallel_for(0, size, [&](int i) {
					aux[i].calc_loop(free);
				});

				concurrency::parallel_for(0, size, [&](int i) {
					aux[i].handleCollisions(x_bound, y_bound, z_bound);
				});

				//	atualizando as posições para o proximo timestep
				free = aux;
			}
			// serial
			else {
				for (int i = 0; i < free.size(); i++) {
					aux[i].clean_density();
				}


				for (int i = 0; i < free.size(); i++) {
					aux[i].def_density(free);
				}


				for (int i = 0; i < free.size(); i++) {
					aux[i].calc_loop(free);
				}

				for (int i = 0; i < free.size(); i++) {
					aux[i].handleCollisions(x_bound, y_bound, z_bound);
				}

				//	atualizando as posições para o proximo timestep
				free = aux;
			}
		}

		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// ativar shader antes de settar seus dados
		lightShader.use();

		lightShader.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
		lightShader.setVec3("viewPos", camera.Position);

		//light properties
		lightShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
		lightShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
		lightShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

		//material properties
		lightShader.setFloat("material.shininess", 32.0f);

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		lightShader.setMat4("projection", projection);
		lightShader.setMat4("view", view);

		// colocando cada partcula em seu lugar
		for (int i = 0; i < free.size(); i++) {
			glm::mat4 model;
			model = glm::translate(model, free[i].pos); 
			model = glm::scale(model, glm::vec3(scaling_factor, scaling_factor, scaling_factor));
			lightShader.setMat4("model", model);
			fluid_m.Draw(lightShader);
		}
		
		
		//	esse bloco salva os frames capturados em arquivos .tga
		//	depois uso esses frames para montar um video com ffmpeg
		string str;
		if (curFrame < 10) {
			str = "C:/Users/dquei/Desktop/Output/frame00" + std::to_string(curFrame) + ".tga";
		}
		else if (curFrame < 100) {
			str = "C:/Users/dquei/Desktop/Output/frame0" + std::to_string(curFrame) + ".tga";
		}
		else {
			str = "C:/Users/dquei/Desktop/Output/frame" + std::to_string(curFrame) + ".tga";
		}
		char * filename = new char[str.size() + 1];
		std::copy(str.begin(), str.end(), filename);
		filename[str.size()] = '\0';
		cappie.screenshot(filename, SCR_WIDTH, SCR_HEIGHT);
		delete[] filename;

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	std::time_t t2 = time(nullptr);
	std::cout << "tempo total: " << t2 - t1 << std::endl;
	getchar(); // pra ler o output do console sem ter que salvar num arquivo
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		simulate = true;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
