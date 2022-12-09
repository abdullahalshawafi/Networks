//
// Generated file, do not edit! Created by opp_msgtool 6.0 from Packet.msg.
//

#ifndef __PACKET_M_H
#define __PACKET_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// opp_msgtool version check
#define MSGC_VERSION 0x0600
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgtool: 'make clean' should help.
#endif

class Packet;
/**
 * Class generated from <tt>Packet.msg:1</tt> by opp_msgtool.
 * <pre>
 * packet Packet
 * {
 *     \@customize(true);
 *     int header;
 *     string payload;
 *     char trailer;
 *     short frame_type;
 *     int ACK_nr;
 * 
 * }
 * </pre>
 *
 * Packet_Base is only useful if it gets subclassed, and Packet is derived from it.
 * The minimum code to be written for Packet is the following:
 *
 * <pre>
 * class Packet : public Packet_Base
 * {
 *   private:
 *     void copy(const Packet& other) { ... }

 *   public:
 *     Packet(const char *name=nullptr, short kind=0) : Packet_Base(name,kind) {}
 *     Packet(const Packet& other) : Packet_Base(other) {copy(other);}
 *     Packet& operator=(const Packet& other) {if (this==&other) return *this; Packet_Base::operator=(other); copy(other); return *this;}
 *     virtual Packet *dup() const override {return new Packet(*this);}
 *     // ADD CODE HERE to redefine and implement pure virtual functions from Packet_Base
 * };
 * </pre>
 *
 * The following should go into a .cc (.cpp) file:
 *
 * <pre>
 * Register_Class(Packet)
 * </pre>
 */
class Packet_Base : public ::omnetpp::cPacket
{
  protected:
    int header = 0;
    omnetpp::opp_string payload;
    char trailer = 0;
    short frame_type = 0;
    int ACK_nr = 0;

  private:
    void copy(const Packet_Base& other);

  protected:
    bool operator==(const Packet_Base&) = delete;
    // make assignment operator protected to force the user override it
    Packet_Base& operator=(const Packet_Base& other);

  public:
    Packet_Base(const char *name=nullptr, short kind=0);
    Packet_Base(const Packet_Base& other);
    virtual ~Packet_Base();
    virtual Packet_Base *dup() const override {
      return new Packet_Base(*this);
    }
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    virtual int getHeader() const;
    virtual void setHeader(int header);

    virtual const char * getPayload() const;
    virtual void setPayload(const char * payload);

    virtual char getTrailer() const;
    virtual void setTrailer(char trailer);

    virtual short getFrame_type() const;
    virtual void setFrame_type(short frame_type);

    virtual int getACK_nr() const;
    virtual void setACK_nr(int ACK_nr);
};


namespace omnetpp {

template<> inline Packet_Base *fromAnyPtr(any_ptr ptr) { return check_and_cast<Packet_Base*>(ptr.get<cObject>()); }

}  // namespace omnetpp

#endif // ifndef __PACKET_M_H

