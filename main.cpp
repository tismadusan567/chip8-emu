#include <cstdint>
#include <fstream>


class Cpu 
{
public:
    Cpu();
    void cycle();
    void execute(uint16_t opcode);
    void load(char* filename);
private:
    uint8_t Vx[16];
    uint16_t I;
    uint8_t delay;
    uint8_t sound;
    uint16_t pc = 0x200;
    uint8_t sp;
    uint16_t stack[16];
    uint8_t ram[4096];
    const uint32_t start_adress = 0x200;

};

Cpu::Cpu()
{

}

void Cpu::cycle()
{
    uint16_t opcode = ram[pc];
    
}

void Cpu::execute(uint16_t opcode)
{
    
}

void Cpu::load(char* filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        int size = file.tellg();
        char* buf = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(buf, size);
        file.close();
        for(int i = 0;i < size;i++)
        {
            ram[start_adress + i] = buf[i];
        }
        delete[] buf;
    }
}

int main()
{
    // Cpu *cpu = new Cpu();
    // Cpu cpu = Cpu();
    Cpu cpu;
    cpu.load("space.ch8");
    return 0;
}