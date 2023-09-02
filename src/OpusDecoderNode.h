//
// Created by Adam on 5/30/2020.
// Ported to GDExtension by goatchurchprime on 2023-09-02
//

#ifndef OPUS_GDEXTENSION_OPUSDECODERNODE_H
#define OPUS_GDEXTENSION_OPUSDECODERNODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <mutex>
#include <opus.h>


namespace godot
{

class OpusDecoderNode : public Node 
{
    GDCLASS(OpusDecoderNode, Node)

private:

    int frame_size;
    int max_frame_size;
	OpusDecoder *decoder = nullptr;
	int outBuffSize;
	opus_int16 *outBuff = nullptr;

	std::mutex decoder_mutex;

protected:
    static void _bind_methods();

public:
	int sample_rate;
	int pcm_channel_size;
	int channels;

	OpusDecoderNode();
	~OpusDecoderNode();
    void _ready();

    void set_sample_rate(const int p_sample_rate);
    int get_sample_rate() const;
    void set_pcm_channel_size(const int p_pcm_channel_size);
    int get_pcm_channel_size() const;
    void set_channels(const int p_channels);
    int get_channels() const;

    PackedByteArray decode(const PackedByteArray opusEncoded);
};

}

#endif //OPUS_GDEXTENSION_OPUSDECODERNODE_H
