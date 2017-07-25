#include <vector>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>

class Particle {

private:

	/*glm::vec3 pos;
	glm::vec3 velocity;
	glm::vec3 accel;*/

	//the forces
	glm::vec3 pressure = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
	glm::vec3 viscosity = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 surface_tension = glm::vec3(0.0f, 0.0f, 0.0f);

	//constanes e variaveis importantes e seus simbolos nas equacoes associadas
	float mass = 1.0f; // m
	float rest_density = 20.0f; // ρ0
	float gas_constant = 20.0f;  // k
	float viscosity_constant = 0.018f; // μ
	float surface_tension_coefficient = 8.0f; // γ

	float density = 20.0f; // aqui vai ser salva a densidade em cada timestep
	float dT = 0.02f; // time step para 50 quadros por segundo
	float damping = 0.25f; // para colisoes
	float smoothingWidth = 0.53f; // h

	float PI = 3.14159265358979323846f;


public:

	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 accel = glm::vec3(0.0f, 0.0f, 0.0f);

	Particle() {

	}

	void def_density(std::vector<Particle> p)
	{
		float resp = 0.0f;
		for (int i = 0; i < p.size(); i++) {
			glm::vec3 direction = this->pos - p[i].pos;
			resp += p[i].mass * p[i].poly6_kernel(direction.length(), smoothingWidth);
		}
		this->density += resp;
	}

	// eq (12)
	void get_pressure_vector(std::vector<Particle> p, float h)
	{
		glm::vec3 resp = glm::vec3(0.0f, 0.0f, 0.f);
		glm::vec3 tmp = glm::vec3(0.0f, 0.0f, 0.f);

		for (int i = 0; i < p.size(); i++) {
			glm::vec3 direction = this->pos - p[i].pos;
			glm::vec3 normalizedDirection = glm::normalize(direction);
			tmp = p[i].mass * ((this->get_pressure_value() + p[i].get_pressure_value()) / 2.0f * (this->density + p[i].density)) * normalizedDirection * spiky_kernel(direction.length(), smoothingWidth);
			if (!isnan(tmp.x) && !isnan(tmp.y) && !isnan(tmp.z)) {
				resp += tmp;
			}
		}
		resp *= -1.0f;
		pressure += resp;
	}

	// eq (10)
	void get_viscosity_vector(std::vector<Particle> p, float h)
	{
		glm::vec3 resp = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 tmp = glm::vec3(0.0f, 0.0f, 0.0f);

		for (int i = 0; i < p.size(); i++)
		{
			glm::vec3 direction = this->pos - p[i].pos;

			tmp = ((p[i].velocity - this->velocity) / p[i].density) * viscosity_kernel(direction.length(), smoothingWidth);
			if (isnan(tmp.x) || isnan(tmp.y) || isnan(tmp.z)) {
				std::cout << "viscosity is NaN avoided!!!!" << std::endl;
			}
			else {
				resp += tmp;
			}
		}
		resp *= viscosity_constant;

		if (isnan(resp.x) || isnan(resp.y) || isnan(resp.z)) {
			std::cout << "viscosity is NaN detected inside function!!!!" << std::endl;
		}

		this->viscosity = resp;
	}

	// todo esse bloco para definir a tensao superficial tem que ser implementado melhor
	//// eq (18)
	//glm::vec3 get_surface_tension_vector(std::vector<Particle> p, float h)
	//{
	//	glm::vec3 resp = glm::vec3(0.0f, 0.0f, 0.0f);
	//	return resp;
	//}

	//glm::vec3 get_cohesion_vector()
	//{

	//}

	//glm::vec3 get_curvature_vector(std::vector<Particle> p, float h) {
	//	glm::vec3 resp = glm::vec3(0.0f, 0.0f, 0.0f);

	//	//resp = -1.0f * surface_tension_coefficient * this->mass * (get_particle_for_curvature(this))

	//	return resp;
	//}

	//glm::vec3 get_particle_for_curvature(std::vector<Particle> p, float h) {
	//	glm::vec3 resp = glm::vec3(0.0f, 0.0f, 0.0f);

	//	for (int i = 0; i < p.size(); i++)
	//	{
	//		glm::vec3 direction = this->pos - p[i].pos;
	//		resp += (p[i].mass / p[i].density) * nabla_poly6_kernel(distance(p[i].pos), h);
	//	}

	//	resp *= h;

	//	return resp;
	//}

	//float correctional_factor(float densityThis, float densityThat)
	//{
	//	return ((2 * this->rest_density) / densityThis + densityThat);
	//}

	void get_accel_vector() {
		this->accel = this->pressure + this->viscosity + this->gravity;
	}

	void get_velocity_vector() {
		glm::vec3 vel;
		vel = velocity + accel*dT;
		velocity = vel;
	}

	void update_pos() {
		glm::vec3 p;
		p = pos + velocity * dT;
		pos = p;
	}

	//calcula sem parede
	void calc_loop(std::vector<Particle> free) {

		pressure = glm::vec3(0.0f, 0.0f, 0.0f);
		viscosity = glm::vec3(0.0f, 0.0f, 0.0f);

		//	calculando as forças
		get_pressure_vector(free, smoothingWidth);
		//get_viscosity_vector(free, smoothingWidth); // tem algo dando errado aqui
		if (isnan(viscosity.x) || isnan(viscosity.y) || isnan(viscosity.z)) {
			std::cout << "viscosity is NaN detected!!!!" << std::endl;
		}

		get_accel_vector();

		get_velocity_vector();

		update_pos();
		if (isnan(pos.x) || isnan(pos.y) || isnan(pos.z)) {
			std::cout << "NaN detected!!!!" << std::endl;
		}
	}

	float get_pressure_value() {

		//return (-1.0f) * ((pow(density / rest_density, 7.0f) - 1.0f) * gas_constant);
		float resp = gas_constant * (density - rest_density);
		resp = (resp < rest_density) ? rest_density : resp;
		return resp;
	}

	void clean_density() {
		this->density = 0.0f;
	}

	void handleCollisions(float maxX, float maxY, float maxZ) {
		if (this->pos.x < 0.0f) {
			this->pos.x = 0.0f;
			this->velocity.x = (-1.0f) * this->velocity.x * damping;
		}
		if (this->pos.x > maxX) {
			this->pos.x = maxX;
			this->velocity.x = (-1.0f) * this->velocity.x * damping;
		}

		if (this->pos.y < 0.0f) {
			this->pos.y = 0.0f;
			this->velocity.y = (-1.0f) * this->velocity.y * damping;
		}
		if (this->pos.y > maxY) {
			this->pos.y = maxY;
			this->velocity.y = (-1.0f) * this->velocity.y * damping;
		}

		if (this->pos.z < 0.0f) {
			this->pos.z = 0.0f;
			this->velocity.z = (-1.0f) * this->velocity.z * damping;
		}
		if (this->pos.z > maxZ) {
			this->pos.z = maxZ;
			this->velocity.z = (-1.0f) * this->velocity.z * damping;
		}
	}

	//	kernel functions

	//	deixando esse aqui por garantia, kernel 'burro'
	float kernel(float distance, float h)
	{
		float resp = 0.0f;
		if (distance < h)
		{
			resp = 1.0f - (distance / h);
		}
		return resp;
	}

	float poly6_kernel(float distance, float h)
	{
		float resp = 0.0f;
		if (0 <= distance && distance <= h)
		{
			resp = (315.0f / 64.0f * PI * pow(h, 9))*(pow((pow(h, 2) - pow(distance, 2)), 3));
		}
		return resp;
	}

	float nabla_poly6_kernel(float distance, float h)
	{
		float resp = 0.0f;
		//	aqui eu calculo um pouco diferente, porque eu uso o vetor de direção no cálculo do vetor de pressão, não  no kernel
		if (0 <= distance && distance <= h)
		{
			resp = (945.0f / 32.0f * PI * pow(h, 9)) * (pow((pow(h, 2) - pow(distance, 2)), 2));
		}
		return resp;
	}

	float spiky_kernel(float distance, float h) {
		float resp = 0.0f;

		if (0 <= distance && distance <= h) {
			resp = (-1.0f) * ((45) / PI * pow(h, 6)) *  pow(h - distance, 2);
		}

		return resp;

	}

	float viscosity_kernel(float distance, float h) {
		float resp = 0.0f;

		if (distance > 0 && distance <= h)
		{
			resp = ((-1.0f) * (pow(distance, 3) / 2 * pow(h, 3))) + (pow(distance, 2) / pow(h, 2)) + (h / 2 * distance) - 1.0f;
		}

		return resp;
	}
};
