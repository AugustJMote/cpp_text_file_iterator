# Fast Buffered Text File Iterator.
A simple buffered text file iterator that returns std::optional<std::string_view>s or dereferenceable iterators with a beyond-the-end iterator for a light_weight view into a text file by line.

3x faster then std::ifstream + std::getline().

## Usage

```c_cpp
auto text_file = BufferedTextFile("/proc/self/status");
while(auto line = text_file.get_line())
      std::cout << *line << std::endl;
```
or
```c_cpp
for(auto line: BufferedTextFile("/proc/self/status"))
      std::cout << *line << std::endl;
```
