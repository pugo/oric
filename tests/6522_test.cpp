#include <vector>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../config.hpp"
#include "../oric.hpp"
#include "../machine.hpp"
#include "../chip/mos6522.hpp"
#include "../memory.hpp"


namespace Unittest {

using namespace testing;


class MOS6522Test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        Config config;

        oric = new Oric(config);
        oric->init_machine();
        oric->get_machine().init_mos6522();

        mos6522 = oric->get_machine().mos_6522;
    }

    virtual void TearDown()
    {
        delete oric;
    }

    void run(Machine& machine) {
        bool brk = false;
        while (! brk) {
            machine.cpu->exec_instruction(brk);
        }
    }

    int decToBCD(int dec)
    {
        int major = dec / 10;
        int minor = dec - major*10;
        //std::cout << "D2BCD - major: " << major << ", minor: " << minor << ", result: "
        //          << std::hex << major * 16 + minor << std::endl;
        return major * 16 + minor;
    }

    int bcdToDec(int bcd)
    {
        int major = bcd / 16;
        int minor = bcd - major*16;
        //std::cout << "BCD2D - major: " << major << ", minor: " << minor << ", result: "
        //          << std::hex << major * 16 + minor << std::endl;
        return major * 10 + minor;
    }

    Oric* oric;
    MOS6522* mos6522;
};


//TEST_F(MOS6522Test, WriteReadORA)
//{
//    mos6522->write_byte(MOS6522::ORA, 0xff);
//    ASSERT_EQ(mos6522->read_byte(MOS6522::ORA), 0xff);
//}

TEST_F(MOS6522Test, WriteReadORB)
{
    mos6522->write_byte(MOS6522::ORB, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::ORB), 0xff);
}

TEST_F(MOS6522Test, WriteReadDDRA)
{
    mos6522->write_byte(MOS6522::DDRA, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::DDRA), 0xff);
}

TEST_F(MOS6522Test, WriteReadDDRB)
{
    mos6522->write_byte(MOS6522::DDRB, 0xff);
    ASSERT_EQ(mos6522->read_byte(MOS6522::DDRB), 0xff);
}

} // Unittest