#include "gdexample.h"
#include <godot_cpp/core/class_db.hpp>
#include "Values.h"
#include "Utils.h"

using namespace godot;


GDExample::GDExample() {
    // Initialize any variables here.
    time_passed = 0.0;
    time_emit = 0.0;
    speed = 0.0;
    amplitude = 13.0;
    
    
    frame_size = 0;
    max_frame_size = 0;

    sample_rate = DEFAULT_SAMPLE_RATE;
    pcm_channel_size = sizeof(opus_uint16);
    channels = DEFAULT_CHANNELS;
    
    std::cout << "GDExample -- constructed3\n"; 
}

void GDExample::_ready()
{
    std::lock_guard <std::mutex> guard(decoder_mutex);

    frame_size = sample_rate / 50; // We want a 20ms window
    max_frame_size = frame_size * 6;

    outBuffSize = max_frame_size * channels;
    outBuff = new opus_int16[outBuffSize];

    int err;
    decoder = opus_decoder_create(sample_rate, channels, &err);
    if(err < 0)
    {
        WARN_PRINT("failed to create decoder: {0}\n"); //, opus_strerror(err));
    }
}

GDExample::~GDExample() {
    std::cout << "GDExample -- destructed3\n"; 
    std::lock_guard <std::mutex> guard(decoder_mutex);

    if(decoder != nullptr)
    {
        std::cout << "GDExample -- destructed3-buffer\n"; 
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }

    delete[] outBuff;
    outBuff = nullptr;
}


void GDExample::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_amplitude"), &GDExample::get_amplitude);
    ClassDB::bind_method(D_METHOD("set_amplitude", "p_amplitude"), &GDExample::set_amplitude);
    ClassDB::add_property("GDExample", PropertyInfo(Variant::FLOAT, "amplitude"), "set_amplitude", "get_amplitude");
    ADD_SIGNAL(MethodInfo("position_changed", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::VECTOR2, "new_pos")));


    ClassDB::bind_method(D_METHOD("get_speed"), &GDExample::get_speed);
    ClassDB::bind_method(D_METHOD("set_speed", "p_speed"), &GDExample::set_speed);
    ClassDB::add_property("GDExample", PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "0,20,0.01"), "set_speed", "get_speed");

    ClassDB::bind_method(D_METHOD("decode", "opusEncoded"), &GDExample::decode);
}

void GDExample::_process(double delta) {
    time_passed += delta;

    Vector2 new_position = Vector2(
        amplitude + (amplitude * sin(time_passed * 2.0)),
        amplitude + (amplitude * cos(time_passed * 1.5))
    );

    set_position(new_position);

    time_emit += delta;
    if (time_emit > 1.0) {
        emit_signal("position_changed", this, new_position);
        time_emit = 0.0;
    }
}

void GDExample::set_amplitude(const double p_amplitude) {
    amplitude = p_amplitude;
}

double GDExample::get_amplitude() const {
    return amplitude;
}

void GDExample::set_speed(const double p_speed) {
    speed = p_speed;
}

double GDExample::get_speed() const {
    return speed;
}

PackedByteArray GDExample::decode(const PackedByteArray opusEncoded)
{
    std::lock_guard<std::mutex> guard(decoder_mutex);

    PackedByteArray decodedPcm;

    const int numInputBytes = opusEncoded.size();
    std::cout << " numInputBytes size " << numInputBytes << "\n";

    if(numInputBytes <= 0)
    {
        WARN_PRINT("Opus Decoder: encoded input was empty");
        return decodedPcm;
    }

    // Initial output buffer size for 5 seconds of audio
    const int max_frame_size_bytes = max_frame_size * channels * pcm_channel_size;
    const int framesPerSecond = 50;
    const int initialOutputSize = max_frame_size_bytes * framesPerSecond;
    decodedPcm.resize(initialOutputSize);

    //const PackedByteArray::Read read = opusEncoded.read();
    //const unsigned char *compressedBytes = read.ptr();
    const unsigned char *compressedBytes = reinterpret_cast<const unsigned char *>(opusEncoded.ptr());

    for (int i = 0; i < 10; i++)
        std::cout << " i " << (int)compressedBytes[i] << "\n";
    // should be: [28, 0, 0, 0, 120, 5, 168, 176, 62, 28]
    // was: [0,...,0,96,20]
    for (int i = 0; i < 10; i++)
        std::cout << " id " << (int)opusEncoded[i] << "\n";


    // How far into the input buffer we are
    int byteMark = 0;
    // Keep track of how far into the output buffer we are
    int outByteMark = 0;

    bool done = false;
    while(!done)
    {
        // Clear the buffers
        memset(outBuff, 0, outBuffSize);

        // Parse out packet size
        Bytes4 b{0};
        for(int ii = 0; ii < 4; ++ii) b.bytes[ii] = compressedBytes[byteMark + ii];
        const int packetSize = b.integer;

        byteMark += 4; // Move past the packet size

        // Very unintelligent sanity check to make sure our packet size header wasn't corrupt
        if(packetSize <= 0 || packetSize > 2048)
        {
            WARN_PRINT("Bad packet size, exiting.");
            break;
        }

        // Get pointer to current packet
        const unsigned char *inData = &compressedBytes[byteMark];

        byteMark += packetSize; // move past the packet

        // If this is the last packet, we will exit when we finish this pass
        if(byteMark >= numInputBytes - 5)
        {
            done = true;
        }

        // Decode the current opus packet
        int out_frame_size = opus_decode(decoder, inData, packetSize, outBuff, max_frame_size, 0);
        if(out_frame_size < 0)
        {
            WARN_PRINT(String("decoder failed: {0}").format(Array::make(opus_strerror(out_frame_size))));
            std::cout << " decoder failed \n";
            
            break;
        }

        // Prep output for copy
        const unsigned char *outBytes = reinterpret_cast<unsigned char *>(outBuff);
        const int outBytesSize = out_frame_size * channels * pcm_channel_size;

        // Copy the new data into the output buffer
        opus::ensure_buffer_size(decodedPcm, outByteMark, outBytesSize);
        uint8_t *decodedBytes = reinterpret_cast<uint8_t *>(decodedPcm.ptrw());
        uint8_t *targetArea = &(decodedBytes[outByteMark]);
        memcpy(targetArea, outBytes, outBytesSize);

        // Move the mark past the bytes we just wrote
        outByteMark += outBytesSize;
    }

    // Down size our buffer to the required size
    if(decodedPcm.size() > outByteMark+1)
    {
        decodedPcm.resize(outByteMark+1);
    }
    std::cout << " decodedPcm size " << decodedPcm.size() << "\n";
    
    return decodedPcm;
}


