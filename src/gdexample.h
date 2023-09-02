#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <mutex>
#include <opus.h>

namespace godot {

class GDExample : public Sprite2D {
    GDCLASS(GDExample, Sprite2D)

private:
    double time_passed;
    double time_emit;
    double speed;
    double amplitude;
    
    int frame_size;
    int max_frame_size;
    OpusDecoder *decoder = nullptr;
    int outBuffSize;
    opus_int16 *outBuff = nullptr;

    std::mutex decoder_mutex;
    

protected:
    static void _bind_methods();


public:
    GDExample();
    ~GDExample();


    int sample_rate;
    int pcm_channel_size;
    int channels;


    void _process(double delta);
    void _ready();
    
    
    void set_amplitude(const double p_amplitude);
    double get_amplitude() const;
    void set_speed(const double p_speed);
    double get_speed() const;
    
    PackedByteArray decode(const PackedByteArray opusEncoded);

};

}

#endif
