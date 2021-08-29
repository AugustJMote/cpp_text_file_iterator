#include <memory>
#include <iterator>
#include <cstddef>
#include <optional>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

template<size_t BUFFER_SIZE = 16*1024>
class BufferedTextFile
{
public:
    BufferedTextFile(char const* filename)
    : _fd(open(filename, O_RDONLY)),
    _current_line(0)
    {
        if(_fd < 0)
            throw std::runtime_error("File Open Failed");
        posix_fadvise(_fd, 0, 0, 1);
        _bytes_read = read(_fd, _buf.data(), BUFFER_SIZE);
        _buf[BUFFER_SIZE] = '\0';
        if(_bytes_read <= 0)
            throw std::runtime_error("File Read Failed");
        _current = _buf.data();
    }

    ~BufferedTextFile()
    {
        close(_fd);
    }

    inline std::optional<std::string_view> get_line()
    {
        if(!_remainder.empty())
            _remainder.clear();
            
        char* last = _current;
        _current = reinterpret_cast<char*>(memchr(_current, '\n', _bytes_read - (_current - _buf.data()))) + 1;
        
        if(_current - 1)
        {
            return std::string_view(last, _current - last - 1);
        }

        _remainder = std::string(last, _bytes_read - (last - _buf.data()));
        _bytes_read = read(_fd, _buf.data(), BUFFER_SIZE);
        if(_bytes_read <= 0)
            return {};

        while(!(_current = reinterpret_cast<char*>(memchr(_buf.data(), '\n', _bytes_read))))
        {
            _remainder += std::string(_buf.data(), _bytes_read);
            _bytes_read = read(_fd, _buf.data(), BUFFER_SIZE);
            if(_bytes_read <= 0)
                return _remainder;
        }
        ++_current;
        if(_current - 1)
        {
            if(!_remainder.empty())
            {
                _remainder += std::string(_buf.data(), _current - _buf.data() - 1);
                return _remainder;
            }
            return std::string_view(_buf.data(), _current - _buf.data() - 1);
        }
        return {};
    }

    struct Iterator 
    {
        using iterator_category = std::forward_iterator_tag;

        Iterator(BufferedTextFile& file) : _file(file), _line_num(1)
        {
            auto initial_line = _file.get_line();
            if (!initial_line)
                _line_num = std::numeric_limits<size_t>::max();
            else
                _current_line = *initial_line;
        }

        Iterator(BufferedTextFile& file, size_t line_num) : _file(file), _line_num(line_num)
        {}

        constexpr std::string_view const& operator*() const { return _current_line; }
        constexpr std::string_view* operator->() { return &_current_line; }

        Iterator& operator++()
        {
            auto new_line = _file.get_line();
            if (!new_line)
                _line_num = std::numeric_limits<size_t>::max();
            else
            {
                _current_line = *new_line;
                ++_line_num;
            }
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend constexpr bool operator== (const Iterator& a, const Iterator& b) { return a._line_num == b._line_num; };
        friend constexpr bool operator!= (const Iterator& a, const Iterator& b) { return a._line_num != b._line_num; };   

    private:
        BufferedTextFile& _file;
        std::string_view _current_line;
        size_t _line_num;
    };

    Iterator begin() { return Iterator(*this); }
    Iterator end() { return Iterator(*this, std::numeric_limits<size_t>::max()); }

private:
    int _fd;
    std::array<char, BUFFER_SIZE + 1> _buf;
    size_t _bytes_read;
    size_t _current_line;
    char* _current;
    std::string _remainder;
};
