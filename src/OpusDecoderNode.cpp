#include "OpusDecoderNode.h"
#include <godot_cpp/core/class_db.hpp>
#include "Values.h"
#include "Utils.h"

using namespace godot;


OpusDecoderNode::OpusDecoderNode() {
    frame_size = 0;
    max_frame_size = 0;

    sample_rate = DEFAULT_SAMPLE_RATE;
    pcm_channel_size = sizeof(opus_uint16);
    channels = DEFAULT_CHANNELS;
    
    std::cout << "OpusDecoderNode -- constructed4\n"; 
}

void OpusDecoderNode::_ready()
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

OpusDecoderNode::~OpusDecoderNode() {
    std::cout << "OpusDecoderNode -- destructed4\n"; 
    std::lock_guard <std::mutex> guard(decoder_mutex);

    if(decoder != nullptr)
    {
        std::cout << "OpusDecoderNode -- destructed4-buffer\n"; 
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }

    delete[] outBuff;
    outBuff = nullptr;
}


void OpusDecoderNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sample_rate"), &OpusDecoderNode::get_sample_rate);
    ClassDB::bind_method(D_METHOD("set_sample_rate", "p_sample_rate"), &OpusDecoderNode::set_sample_rate);
    ClassDB::add_property("OpusDecoderNode", PropertyInfo(Variant::INT, "sample_rate"), "set_sample_rate", "get_sample_rate");

    ClassDB::bind_method(D_METHOD("get_pcm_channel_size"), &OpusDecoderNode::get_pcm_channel_size);
    ClassDB::bind_method(D_METHOD("set_pcm_channel_size", "p_pcm_channel_size"), &OpusDecoderNode::set_pcm_channel_size);
    ClassDB::add_property("OpusDecoderNode", PropertyInfo(Variant::INT, "pcm_channel_size"), "set_pcm_channel_size", "get_pcm_channel_size");

    ClassDB::bind_method(D_METHOD("get_channels"), &OpusDecoderNode::get_channels);
    ClassDB::bind_method(D_METHOD("set_channels", "p_channels"), &OpusDecoderNode::set_channels);
    ClassDB::add_property("OpusDecoderNode", PropertyInfo(Variant::INT, "channels"), "set_channels", "get_channels");

    ClassDB::bind_method(D_METHOD("decode", "opusEncoded"), &OpusDecoderNode::decode);
    std::cout << " OpusDecoderNode bind methods run \n";

}


void OpusDecoderNode::set_sample_rate(const int p_sample_rate) { sample_rate = p_sample_rate; }
int OpusDecoderNode::get_sample_rate() const { return sample_rate; }
void OpusDecoderNode::set_channels(const int p_channels) { channels = p_channels; }
int OpusDecoderNode::get_channels() const { return channels; }
void OpusDecoderNode::set_pcm_channel_size(const int p_pcm_channel_size) { pcm_channel_size = p_pcm_channel_size; }
int OpusDecoderNode::get_pcm_channel_size() const { return pcm_channel_size; }


PackedByteArray OpusDecoderNode::decode(const PackedByteArray opusEncoded)
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

    const unsigned char *compressedBytes = reinterpret_cast<const unsigned char *>(opusEncoded.ptr());

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


