
基于单例模式的日志多生产者单消费者的异步日志系统。

生产者将日志消息放入队列，使用一个单线程的消费者线程从队列中取出消息并持续地写入文件。

消息队列有最大长度限制，超过长度限制时，默认直接丢弃旧消息。

日志信息支持不同地日志级别分类，并使用fmt库尝试格式化日志消息。

使用示例：
```cpp
Logger::getInstance()->log(LogLevel::INFO, "Hello {} {}", "world", 123);
```

# 参考资料
[llfc大佬的C++学习资料](https://www.yuque.com/lianlianfengchen-cvvh2/zack/gl4wsbxs7krn8b93#iUp8C)
[fmt官方文档](https://fmt.dev/12.0/)
[单例模式]()
