#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:
	GLFWwindow* window;

	Window(int x, int y, const char* title);
	~Window();
	void Resize();
	int Close();
};


Window::Window(int x, int y, const char* title)
{
	GLFWwindow* window = glfwCreateWindow(x, y, title, NULL, NULL);
	this->window = window;
}


Window::~Window()
{
	glfwDestroyWindow(this->window);
}

void Window::Resize() 
{
	GLint w, h;
	glfwGetWindowSize(this->window, &w, &h);
	glViewport(0, 0, w, h);
}

int Window::Close()
{
	return glfwWindowShouldClose(this->window);
}

