#include <cstdint>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <bitset>
#include <SFML/Graphics.hpp>


class Cpu 
{
public:
    Cpu();
    void cycle();
    void execute(uint16_t opcode);
    void load(const char* filename);
    void loop();
    void print_state();
    void update_display();
    void clear_display();
    sf::Keyboard::Key code_to_key(uint8_t code);
    uint8_t key_to_code();
    bool is_any_key_pressed();
private:
    uint8_t Vx[16];
    uint16_t I = 0;
    uint8_t delay = 0;
    uint8_t sound = 0;
    uint16_t pc = 0x200;
    uint8_t sp = 0;
    uint16_t stack[16];
    uint8_t ram[4096];
    uint8_t display_pixels[64 * 32] = {0};
    std::vector<sf::RectangleShape> display_rectangles;
    int pixel_size = 20;
    const uint32_t start_adress = 0x200;
    sf::RenderWindow window;
};

void test()
{
    Cpu cpu;
    cpu.load("space.ch8");
    cpu.loop();
}

Cpu::Cpu() 
    : window(sf::VideoMode(64 * pixel_size, 32 * pixel_size), "CHIP8")
    , display_rectangles(64*32)
{
    std::srand((unsigned)time(0));
    for(int i=0;i<64;i++)
    {
        for(int j=0;j<32;j++)
        {
            display_rectangles[i + j*64].setSize(sf::Vector2f(float(pixel_size), float(pixel_size)));
            display_rectangles[i + j*64].setFillColor(sf::Color::Blue);
            display_rectangles[i + j*64].setPosition(i*pixel_size,j*pixel_size);
        }
    }

}

void Cpu::update_display()
{
    for(int i=0;i<64;i++)
    {
        for(int j=0;j<32;j++)
        {
            display_rectangles[i + j*64].setFillColor(display_pixels[i + j*64] ? sf::Color::Red : sf::Color::Blue);
        }
    }
}

void Cpu::clear_display()
{
    for(int i=0;i<64;i++)
    {
        for(int j=0;j<32;j++)
        {
            display_pixels[i + j*64] = 0;
        }
    }
}

void Cpu::cycle()
{
    uint16_t opcode = (ram[pc] << 2*4) + ram[pc+1];
    execute(opcode);
}

void Cpu::load(const char* filename)
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

void Cpu::loop()
{
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    // sf::View view(sf::Vector2f(0.f, 0.f),sf::Vector2f(640.f, 320.f));
    // //view.zoom(0.1f);
    // window.setView(view);

    // sf::Image image = sf::Image::create(200, 200, sf::Color::Blue);

    float deltaTime;
    float fps;
    sf::Clock clock;
    std::srand((unsigned)time(0));

    while (window.isOpen())
    {
        //deltaTime / fps
        deltaTime = clock.restart().asSeconds();
        fps = 1.f / deltaTime;

        //events
        sf::Event event; 
        while (window.pollEvent(event))
        {
            switch(event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::Resized:
                    // view.setCenter(window.getView().getCenter());
                    // view.setSize(sf::Vector2f(event.size.width, event.size.height));
                    // window.setView(view);
                    break;
            }
        }
        window.clear();
        if(delay > 0) delay--;
        if(sound > 0)
        {
            printf("\ab");
            sound--;
        }
        cycle();
        update_display();
        for(auto& x : display_rectangles)
        {
            window.draw(x);
        }
        window.display();
    }
}

void Cpu::print_state()
{
    printf("Program counter: %X\n", pc);
    printf("I register: %X\n", I);
    printf("Delay timer: %X\n", delay);
    printf("Sound timer: %X\n", sound);
    printf("### Registers ###\n");
    for(int i=0;i<16;i++)
    {
        printf("\tV%d:\t%X\n", i, Vx[i]);
    }
    printf("\n### Stack ###\n");
    printf("Stack pointer: %X\n", sp);
    for(int i=0;i<16;i++)
    {
        printf("%d:\t%X\n", i, stack[i]);
    }
    printf("------------------------\n");
}




void Cpu::execute(uint16_t opcode)
{
    uint8_t first = (opcode & 0xF000) >> 3*4;
    uint8_t x = (opcode & 0x0F00) >> 2*4;
    uint8_t y = (opcode & 0x00F0) >> 1*4;
    uint8_t last = opcode & 0x000F;
    uint16_t nnn = opcode & 0x0FFF;
    uint16_t kk = opcode & 0x00FF;

    //0 start instructions
    if(first == 0)
    {
        // 00E0 - CLS
        // Clear the display.
        if(opcode == 0x00E0)
        {
            clear_display();
        }
        // 00EE - RET
        // Return from a subroutine.
        if(opcode == 0x00EE)
        {
            pc = stack[sp];
            pc -= 2; //at the end of the function it returns to the original value
            sp--;
        }
    }

    // 1nnn - JP addr
    // Jump to location nnn.
    if(first == 1)
    {
        pc = nnn;
        pc -= 2; //at the end of the function it returns to the original value
    }

    // 2nnn - CALL addr
    // Call subroutine at nnn.
    if(first == 2)
    {
        sp++;
        stack[sp] = pc;
        pc = nnn;
        pc -= 2; //at the end of the function it returns to the original value
    }

    // 3xkk - SE Vx, byte
    // Skip next instruction if Vx = kk.  
    if(first == 3)
    {
        if(Vx[x] == kk) pc += 2;
    }

    // 4xkk - SNE Vx, byte
    // Skip next instruction if Vx != kk.
    if(first == 4)
    {
        if(Vx[x] != kk) pc += 2;
    }

    // 5xy0 - SE Vx, Vy
    // Skip next instruction if Vx = Vy.
    if(first == 5 && last == 0)
    {
        if(Vx[x] == Vx[y]) pc += 2;
    }

    // 6xkk - LD Vx, byte
    // Set Vx = kk.
    if(first == 6)
    {
        Vx[x] = kk;
    }

    // 7xkk - ADD Vx, byte
    // Set Vx = Vx + kk.
    if(first == 7)
    {
        Vx[x] = Vx[x] + kk;
    }

    //8 start instructions
    if(first == 8)
    {
        // 8xy0 - LD Vx, Vy
        // Set Vx = Vy.
        if(last == 0)
        {
            Vx[x] = Vx[y];
        }
        
        // 8xy1 - OR Vx, Vy
        // Set Vx = Vx OR Vy.
        if(last == 1)
        {
            Vx[x] = Vx[x] | Vx[y];
        }

        // 8xy2 - AND Vx, Vy
        // Set Vx = Vx AND Vy.
        if(last == 2)
        {
            Vx[x] = Vx[x] & Vx[y];
        }

        // 8xy3 - XOR Vx, Vy
        // Set Vx = Vx XOR Vy.
        if(last == 3)
        {
            Vx[x] = Vx[x] ^ Vx[y];
        }

        // 8xy4 - ADD Vx, Vy
        // Set Vx = Vx + Vy, set VF = carry.
        if(last == 4)
        {
            uint16_t sum = Vx[x] + Vx[y];
            if(sum > 0x00FF) Vx[0xF] = 1;
            sum = sum & 0x00FF;
            Vx[x] = sum;
        }

        // 8xy5 - SUB Vx, Vy
        // Set Vx = Vx - Vy, set VF = NOT borrow.
        if(last == 5)
        {
            if(Vx[x] > Vx[y]) Vx[0xF] = 1;
            else Vx[0xF] = 0;
            Vx[x] = Vx[x] - Vx[y];
        }

        // 8xy6 - SHR Vx {, Vy}
        // Set Vx = Vx SHR 1.
        if(last == 6)
        {
            if(Vx[x] & 0x01 == 0x01) Vx[0xF] = 1;
            else Vx[0xF] = 0;
            Vx[x] >>= 1;
        }
        // 8xy7 - SUBN Vx, Vy
        // Set Vx = Vy - Vx, set VF = NOT borrow.
        if(last == 7)
        {
            if(Vx[y] > Vx[x]) Vx[0xF] = 1;
            else Vx[0xF] = 0;
            Vx[x] = Vx[y] - Vx[x];
        }

        // 8xyE - SHL Vx {, Vy}
        // Set Vx = Vx SHL 1.
        if(last == 0xE)
        {
            if((Vx[x] & 0x80) == 0x80) Vx[0xF] = 1;
            else Vx[0xF] = 0;
            Vx[x] <<= 1;
        }
    }

    // 9xy0 - SNE Vx, Vy
    // Skip next instruction if Vx != Vy.
    if(first == 9 && last == 0)
    {
        if(Vx[x] != Vx[y])
        {
            pc += 2;
        }
    }

    // Annn - LD I, addr
    // Set I = nnn.
    if(first == 0xA)
    {
        I = nnn;
    }

    // Bnnn - JP V0, addr
    // Jump to location nnn + V0.
    if(first == 0xB)
    {
        pc = nnn + Vx[0];
        pc -= 2; //at the end of the function it returns to the original value
    }

    // Cxkk - RND Vx, byte
    // Set Vx = random byte AND kk.
    if(first == 0xC)
    {
        Vx[x] = (rand() % 0xFF) & kk;
    }

    // Dxyn - DRW Vx, Vy, nibble
    // Display n-byte sprite starting at memory location I 
    // at (Vx, Vy), set VF = collision
    if(first == 0xD)
    {
        Vx[0xF] = 0;
        for(int i=0;i<last;i++)
        {
            uint8_t byte = ram[I+i];
            std::bitset<8> bset(byte); //bitset stores in reverse order
            for(int j=0;j<8;j++)
            {
                if(display_pixels[(Vx[x]+j)%64 + ((Vx[y]+i)%32)*64] && bset[7-j]) Vx[0xF] = 1;
                display_pixels[(Vx[x]+j)%64 + ((Vx[y]+i)%32)*64] ^= bset[7-j];
            }
        }
    }

    // Ex9E - SKP Vx
    // Skip next instruction if key with the value of Vx is pressed.
    if(first == 0xE && kk == 0x9E)
    {
        if(sf::Keyboard::isKeyPressed(code_to_key(Vx[x]))) pc += 2;
    }

    // ExA1 - SKNP Vx
    // Skip next instruction if key with the value of Vx is not pressed.
    if(first == 0xE && kk == 0xA1)
    {
        if(!sf::Keyboard::isKeyPressed(code_to_key(Vx[x]))) pc += 2;
    }

    //F start instructions
    if(first == 0xF)
    {
        // Fx07 - LD Vx, DT
        // Set Vx = delay timer value.
        if(kk == 0x07)
        {
            Vx[x] = delay;
        }

        // Fx0A - LD Vx, K
        // Wait for a key press, store the value of the key in Vx.
        if(kk == 0x0A)
        {
            while(!is_any_key_pressed());
            Vx[x] = key_to_code();

        }

        // Fx15 - LD DT, Vx
        // Set delay timer = Vx.
        if(kk == 0x15)
        {
            delay = Vx[x];
        }

        // Fx18 - LD ST, Vx
        // Set sound timer = Vx.
        if(kk == 0x18)
        {
            sound = Vx[x];
        }

        // Fx1E - ADD I, Vx
        // Set I = I + Vx.
        if(kk == 0x1E)
        {
            I = I + Vx[x];
        }

        // Fx29 - LD F, Vx
        // Set I = location of sprite for digit Vx.
        if(kk == 0x29)
        {

        }

        // Fx33 - LD B, Vx
        // Store BCD representation of Vx in memory locations I, I+1, and I+2.
        if(kk == 0x33)
        {
            ram[I] = Vx[x] / 100;
            ram[I+1] = (Vx[x] % 100) / 10;
            ram[I+2] = Vx[x] % 10;
        }

        // Fx55 - LD [I], Vx
        // Store registers V0 through Vx in memory starting at location I.
        if(kk == 0x55)
        {
            for(int i=0;i<=x;i++)
            {
                ram[I+i] = Vx[i];
            }
        }

        // Fx65 - LD Vx, [I]
        // Read registers V0 through Vx from memory starting at location I.
        if(kk == 0x65)
        {
            for(int i=0;i<=x;i++)
            {
                Vx[i] = ram[I+i];
            }
        }
    }

    //move to the next instruction
    pc += 2;
    //print_state();
}

bool Cpu::is_any_key_pressed()
	{
		for (int k = -1; k < sf::Keyboard::KeyCount; ++k)
		{
			if (sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(k)))
				return true;
		}
		return false;
	}

sf::Keyboard::Key Cpu::code_to_key(uint8_t code)
{
    switch(code)
    {
        case 0x1:
            return sf::Keyboard::Num1;
        case 0x2:
            return sf::Keyboard::Num2; 
        case 0x3:
            return sf::Keyboard::Num3;
        case 0xC:
            return sf::Keyboard::Num4;
        case 0x4:
            return sf::Keyboard::Q;
        case 0x5:
            return sf::Keyboard::W;
        case 0x6:
            return sf::Keyboard::E;
        case 0xD:
            return sf::Keyboard::R;
        
        case 0x7:
            return sf::Keyboard::A;
        case 0x8:
            return sf::Keyboard::S; 
        case 0x9:
            return sf::Keyboard::D;
        case 0xE:
            return sf::Keyboard::F;
        case 0xA:
            return sf::Keyboard::Z;
        case 0x0:
            return sf::Keyboard::X;
        case 0xB:
            return sf::Keyboard::C;
        case 0xF:
            return sf::Keyboard::V;
        default :
            return sf::Keyboard::Unknown;
    }
}

uint8_t Cpu::key_to_code()
{
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num1))
            return 0x1;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num2))
            return 0x2; 
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num3))
            return 0x3;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num4))
            return 0xC;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
            return 0x4;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            return 0x5;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::E))
            return 0x6;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::R))
            return 0xD;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            return 0x7;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            return 0x8; 
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            return 0x9;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::F))
            return 0xE;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
            return 0xA;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::X))
            return 0x0;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::C))
            return 0xB;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::V))
            return 0xF;

        return 0x0;
}

int main()
{
    test();
    return 0;
}