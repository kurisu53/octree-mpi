#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>

#define TINYPLY_IMPLEMENTATION
#include "tinyply.h"
using namespace tinyply;
#include "my_octree.cpp"

std::vector<uint8_t> read_file_binary(const std::string& pathToFile)
{
    std::ifstream file(pathToFile, std::ios::binary);
    std::vector<uint8_t> fileBufferBytes;

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        size_t sizeBytes = file.tellg();
        file.seekg(0, std::ios::beg);
        fileBufferBytes.resize(sizeBytes);
        if (file.read((char*)fileBufferBytes.data(), sizeBytes)) return fileBufferBytes;
    }
    else throw std::runtime_error("could not open binary ifstream to path " + pathToFile);
    return fileBufferBytes;
}

struct memory_buffer : public std::streambuf
{
    char* p_start{ nullptr };
    char* p_end{ nullptr };
    size_t size;

    memory_buffer(char const* first_elem, size_t size)
        : p_start(const_cast<char*>(first_elem)), p_end(p_start + size), size(size)
    {
        setg(p_start, p_start, p_end);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override
    {
        if (dir == std::ios_base::cur) gbump(static_cast<int>(off));
        else setg(p_start, (dir == std::ios_base::beg ? p_start : p_end) + off, p_end);
        return gptr() - p_start;
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
    {
        return seekoff(pos, std::ios_base::beg, which);
    }
};

struct memory_stream : virtual memory_buffer, public std::istream
{
    memory_stream(char const* first_elem, size_t size)
        : memory_buffer(first_elem, size), std::istream(static_cast<std::streambuf*>(this)) {}
};

class manual_timer
{
    std::chrono::high_resolution_clock::time_point t0;
    double timestamp{ 0.0 };
public:
    void start() { t0 = std::chrono::high_resolution_clock::now(); }
    void stop() { timestamp = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() * 1000.0; }
    const double& get() { return timestamp; }
};

int main()
{
    try {
        std::cout << "........................................................................\n";
        std::cout << "Now Reading: " << std::endl;

        std::unique_ptr<std::istream> file_stream;
        std::vector<uint8_t> byte_buffer;

        byte_buffer = read_file_binary("skull.ply");
        file_stream.reset(new memory_stream((char*)byte_buffer.data(), byte_buffer.size()));

        if (!file_stream || file_stream->fail()) throw std::runtime_error("file_stream failed to open ");

        file_stream->seekg(0, std::ios::end);
        const float size_mb = file_stream->tellg() * float(1e-6);
        file_stream->seekg(0, std::ios::beg);

        PlyFile fd;
        fd.parse_header(*file_stream);

        std::cout << "\t[ply_header] Type: " << (fd.is_binary_file() ? "binary" : "ascii") << std::endl;
        for (const auto& c : fd.get_comments()) std::cout << "\t[ply_header] Comment: " << c << std::endl;
        for (const auto& c : fd.get_info()) std::cout << "\t[ply_header] Info: " << c << std::endl;

        for (const auto& e : fd.get_elements())
        {
            std::cout << "\t[ply_header] element: " << e.name << " (" << e.size << ")" << std::endl;
            for (const auto& p : e.properties)
            {
                std::cout << "\t[ply_header] \tproperty: " << p.name << " (type=" << tinyply::PropertyTable[p.propertyType].str << ")";
                if (p.isList) std::cout << " (list_type=" << tinyply::PropertyTable[p.listType].str << ")";
                std::cout << std::endl;
            }
        }

        std::shared_ptr<PlyData> vertices;

        // The header information can be used to programmatically extract properties on elements
        // known to exist in the header prior to reading the data. For brevity of this sample, properties 
        // like vertex position are hard-coded: 
        try { vertices = fd.request_properties_from_element("vertex", { "x", "y", "z" }); }
        catch (const std::exception& e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

        manual_timer read_timer;
        read_timer.start();
        fd.read(*file_stream);
        read_timer.stop();
        const float parsing_time = static_cast<float>(read_timer.get()) / 1000.f;
        std::cout << "\tparsing " << size_mb << "mb in " << parsing_time << " seconds [" << (size_mb / parsing_time) << " MBps]" << std::endl;

        if (vertices)   std::cout << "\tRead " << vertices->count << " total vertices " << std::endl;

        const size_t numVerticesBytes = vertices->buffer.size_bytes();
        vector<Point> verts(vertices->count);
        memcpy(verts.data(), vertices->buffer.get(), numVerticesBytes);

        for (int i = 0; i < 100; i++) {
            std::cout << i << " point: " << verts[i].x << ' ' << verts[i].y << ' ' << verts[i].z << '\n';
        }

        Octree testOctree;
        manual_timer build_timer;
        build_timer.start();
        testOctree.build(verts);
        build_timer.stop();
        const float building_time = static_cast<float>(build_timer.get()) / 1000.f;
        std::cout << "\tbuilding in " << building_time << " seconds [" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught tinyply exception: " << e.what() << std::endl;
    }
}
