#pragma once

namespace armarow {
namespace mac {
namespace simple802_15_4
{
    /** \brief IEEE frame type definitions **/
    enum FrameType {
        BEACON  = 0,        /**< Frame is a PAN-Beacon**/
        DATA    = 1,        /**< Frame contains application data**/
        ACK     = 2,        /**< Frame is an acknowledgement to another transmitted frame**/
        COMMAND = 3         /**< Frame is a command of the mac protocol**/
    };

    /** \brief IEEE adressing modes **/
    enum AdressingMode {
        NONE     = 0,       /**< no address given**/
        RESERVED = 1,       /**< reserved**/
        SHORT    = 2,       /** 2 byte short node-id with 2 byte PAN-id **/
        FULL     = 3        /** 8 byte node-id with 2 byte PAN-id **/
    };

    /** \brief IEEE control field of a IEEE MAC frame**/
    struct ControlField {
        union{
            struct{
                FrameType     type       : 3;   /**< type of the frame**/
                bool          security   : 1;   /**< security extension enabled?**/
                bool          pending    : 1;   /**< pending frame?**/
                bool          ackRequest : 1;   /**< acknowledgement for this frame rquested**/
                bool          intraPAN   : 1;   /**< no routing?**/
                uint8_t                  : 3;   /**< reserved and should be set to zero**/
                AdressingMode dstAdrMode : 2;   /**< adressing mode of destination**/
                uint8_t       version    : 2;   /**< 0x00 ieee 802.15.4-2003 0x01 ieee 802.15.4-2006**/
                AdressingMode srcAdrMode : 2;   /**< addressing mode of source**/
            };
            uint16_t value;
        };
        /** \brief Default constructor
         *
         *  calls reset()
         **/
        ControlField() {
            reset();
        }

        /** \brief resets control information to default values
         *
         *  initializes the message to be a DATA frame with SHORT adressing.
         *  Also all other options are disabled
         **/
        void reset() {
            value      = 0x0000;
            type       = DATA;
            dstAdrMode = SHORT;
            srcAdrMode = SHORT;
        }
    private:
        static const char* frameTypeToString(FrameType type)
        {
            switch(type)
            {
                case(BEACON):  return "Beacon";
                case(DATA):    return "Data";
                case(ACK):     return "Acknowledgement";
                case(COMMAND): return "Command";
                default:       return "Reserved";
            }
        }

        static const char* addressingModeToString(AdressingMode mode)
        {
            switch(mode)
            {
                case(SHORT): return "short with pan";
                default:     return "unknown";
            }
        }
    public:
        void log()
        {
            log::emit<log::Info>()
                << "   type      : " << frameTypeToString(type) << log::endl
                << "   security  : " << (security?"enabled":"disabled") << log::endl
                << "   pending   : " << (pending?"yes":"no") << log::endl
                << "   ackRequest: " << (ackRequest?"yes":"no") << log::endl
                << "   intraPAN  : " << (intraPAN?"yes":"no") << log::endl
                << "   dstAdrMode: " << addressingModeToString(dstAdrMode) << log::endl
                << "   srcAdrMode: " << addressingModeToString(srcAdrMode) << log::endl;
        }

    } __attribute__((packed));

    /** \brief IEEE address in no adressing mode **/
    struct NoAddress
    {
        void log() {
        log::emit<log::Info>() << "   no address" << log::endl;
        }
    };

    /** \brief IEEE address in short adressing mode **/
    struct ShortAddress
    {
        uint16_t pan;
        uint16_t id;

        void log() {
            log::emit<log::Info>()
                << "   pan: " << log::hex << pan << log::endl
                << "   id : " << log::hex << id  << log::endl;
        }
    };

    /** \brief IEEE address in full adressing mode **/
    struct FullAddress
    {
        uint16_t pan;
        uint64_t id;

        void log() {
            log::emit<log::Info>()
                << "   pan: " << log::hex << pan << log::endl
                << "   id : " << log::hex << id << log::endl;
        }
    };

    /** \brief IEEE MAC frame header
     *
     * currently only the short addressing mode is supported!
     **/
    template<typename config>
    struct FrameHeader {
        ControlField control;
        uint8_t seqNumber;
        ShortAddress destination;
        ShortAddress source;

        /** \brief log content of frame header to log level log::Info **/
        void log() {
            log::emit<log::Info>()
                << "MAC-Header:" << log::endl
                << "  control:"  << log::endl;
            control.log();
            log::emit<log::Info>()
                << "  destination: " << log::endl;
            destination.log();
            log::emit<log::Info>()
                << "  source: " << log::endl;
            source.log();
        }
    } __attribute__((packed));
}
}
}
