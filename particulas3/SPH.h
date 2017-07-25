#ifndef SPH_H
#define SPH_H

#define _USE_MATH_DEFINES

#include "particle.h"
#include <math.h>
#include <iostream>
#include <glm/glm.hpp>

class SPH {
public:

	std::vector<Particle> p;

	void update_particles(std::vector<Particle> pa) 
	{
		p = pa;
	}

	SPH(std::vector<Particle> par) 
	{
		p = par;
	}

	double poly6_kernel(glm::dvec3 dir, double h)
	{
		double resp = 0.0;
		double rsq = glm::dot(dir, dir);
		double hsq = h*h;
		if ((0.0 <= rsq) && (rsq <= h))
		{
			resp = (315.0 / 64.0 * M_PI * pow(h, 9))*(pow((hsq - rsq), 3));
		}
		return resp;
	}

	glm::dvec3 poly6_gradient_kernel(glm::dvec3 dir, double h)
	{
		glm::dvec3 resp = dir;

		resp *= 945.0 / (32.0*M_PI*pow(h, 9))*(h*h - glm::length(dir)*glm::length(dir))*(h*h - glm::length(dir)*glm::length(dir));

		return resp;
	}

	glm::dvec3 spiky_kernel_gradient(glm::dvec3 dir, double h)
	{
		glm::dvec3 resp = glm::dvec3(0, 0, 0);
		double r = glm::length(dir);


		if ((0.0 <= r) && (r <= h)) {
			resp = dir;
			resp *= (-1.0) * ((45.0) / M_PI * pow(h, 6)) *  pow(h - r, 2);
		}

		return resp;

	}

	double viscosity_laplacian_kernel(glm::dvec3 dir, double h)
	{
		double r = glm::length(dir);
		double resp = 0;

		//check for neighbourhood
		if ((0 <= r) && (r <= h))
		{
			resp = 45 * (h - r) / (M_PI*pow(h, 6));
		}
		return resp;
	}

	double viscosity_kernel(glm::dvec3 dir, double h)
	{
		double resp = 0.0;
		double r = glm::length(dir);

		if ((0 <= r) && (r <= h))
		{
			resp = ((-1.0f) * (pow(r, 3) / 2 * pow(h, 3))) + (pow(r, 2) / pow(h, 2)) + (h / 2 * r) - 1.0f;
		}

		return resp;
	}

	void compute_density(double h)
	{
		for (int i = 0; i < p.size(); i++) {
			p[i].density = 0;
			for (int j = 0; j < p.size(); j++) {
				p[i].density += p[j].mass * poly6_kernel(p[i].pos - p[j].pos, h);
			}
		}
	}

	void compute_pressure(double h)
	{
		for (int i = 0; i < p.size(); i++) {
			p[i].pressure_value = p[i].gas_constant * (p[i].density - p[i].rest_density);
		}
	}

	void apply_pressure(double h)
	{
		for (int i = 0; i < p.size(); i++) {
			for (int j = 0; j < p.size(); j++) {
				p[i].forces -= p[j].mass * (p[i].pressure_value + p[j].pressure_value) / (2.0 * p[j].density) * spiky_kernel_gradient(p[i].pos - p[j].pos, h);
			}
		}
	}

	void apply_viscosity(double h, double gamma)
	{
		for (int i = 0; i < p.size(); i++) {
			for (int j = 0; j < p.size(); j++) {
				p[i].forces += gamma * p[j].mass * (p[j].velocity - p[i].velocity) / p[j].density * viscosity_laplacian_kernel(p[i].pos - p[j].pos, h);
			}
		}
	}

	void calculate_acceleration(double dt)
	{
		for (int i = 0; i < p.size(); i++) {
			p[i].accel = p[i].forces / p[i].density;
			p[i].accel += dt * glm::dvec3(0.0, -9.8, 0.0);
		}
	}

	void update_velocity(double dt)
	{
		for (int i = 0; i < p.size(); i++) {
			p[i].velocity += p[i].accel * dt;
		}
	}

	void reset_forces()
	{
		for (int i = 0; i < p.size(); i++) {
			p[i].forces = glm::dvec3(0.0, 0.0, 0.0);
		}
	}

	std::vector<Particle> update_particles(double h, double gamma, double dt, double damping, double maxX, double maxY, double maxZ)
	{
		std::cout << "posicao antes p1: " << p[1].pos.x << ", " << p[1].pos.y << ", " << p[1].pos.z << std::endl;
		compute_density(h);
		compute_density(h);

		reset_forces();

		apply_pressure(h);
		apply_viscosity(h, gamma);

		calculate_acceleration(dt);
		update_velocity(dt);

		for (int i = 0; i < p.size(); i++)
		{
			p[i].pos += p[i].velocity * dt;
		}
		std::cout << "posicao depois p1: " << p[1].pos.x << ", " << p[1].pos.y << ", " << p[1].pos.z << std::endl;
		handle_collisions(damping, maxX, maxY, maxZ);
		std::cout << "posicao depois colisao p1: " << p[1].pos.x << ", " << p[1].pos.y << ", " << p[1].pos.z << std::endl;
		return p;

	}

	void handle_collisions(double damping, double maxX, double maxY, double maxZ) {
		for (int i = 0; i < p.size(); i++) {
			if (p[i].pos.x < 0.0) {
				p[i].pos.x = 0.0;
				p[i].velocity.x = (-1.0) * p[i].velocity.x * damping;
			}
			if (p[i].pos.x > maxX) {
				p[i].pos.x = maxX;
				p[i].velocity.x = (-1.0) * p[i].velocity.x * damping;
			}

			if (p[i].pos.y < 0.0) {
				p[i].pos.y = 0.0;
				p[i].velocity.y = (-1.0) * p[i].velocity.y * damping;
			}
			if (p[i].pos.y > maxY) {
				p[i].pos.y = maxY;
				p[i].velocity.y = (-1.0) * p[i].velocity.y * damping;
			}

			if (p[i].pos.z < 0.0) {
				p[i].pos.z = 0.0;
				p[i].velocity.z = (-1.0) * p[i].velocity.z * damping;
			}

			if (p[i].pos.z > maxZ) {
				p[i].pos.z = maxZ;
				p[i].velocity.z = (-1.0) * p[i].velocity.z * damping;
			}
		}
	}
};
#endif // !SPH_H