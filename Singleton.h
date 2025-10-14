#pragma once

#include <iostream>
#include <memory>
#include <mutex>

template<typename T>
class Singleton {
	
public:
	~Singleton();

	// 继承子类需要调用，设置为public
	static std::shared_ptr<T> getInstance();
	static void printAddress() {
		std::cout << "Singleton Address: " << static_instance.get() << std::endl;
	}
	
protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton(const Singleton<T>&&) = delete;
	Singleton<T>& operator=(const Singleton<T>&) = delete;
	Singleton<T>& operator=(const Singleton<T>&&) = delete;
	static std::shared_ptr<T> static_instance;

private:
	static std::once_flag static_flag;
};


template<typename T>
std::shared_ptr<T> Singleton<T>::static_instance;

template<typename T>
std::once_flag Singleton<T>::static_flag;

template<typename T>
inline Singleton<T>::~Singleton()
{
	std::cout << "delete Singleton" << std::endl;
}

template<typename T>
inline std::shared_ptr<T> Singleton<T>::getInstance()
{
	std::call_once(static_flag, []() {
		static_instance = std::shared_ptr<T>(new T);
		});

	return static_instance;
}
