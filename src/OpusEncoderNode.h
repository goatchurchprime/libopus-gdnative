//
// Created by Adam on 5/30/2020.
// Ported to GDExtension by goatchurchprime on 2023-09-02
//

#ifndef OPUS_GDEXTENSION_OPUSENCODERNODE_H
#define OPUS_GDEXTENSION_OPUSENCODERNODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <mutex>
#include <opus.h>
#include "Values.h"

namespace godot
{

class OpusEncoderNode : public Node 
{
    GDCLASS(OpusEncoderNode, Node)

	OpusEncoder *encoder = nullptr;
	int inputSamplesSize;
	opus_int16 *inputSamples = nullptr;
	unsigned char outBuff[sizeof(opus_int16) * MAX_PACKET_SIZE];

	std::mutex encoder_mutex;

	/**
	 * Size of each PCM frame in number of samples
	 */
	int frame_size;
	int application;
	int sample_rate;
	int pcm_channel_size;
	int channels;
	int max_frame_size;

protected:
    static void _bind_methods();

public:
	int bit_rate;

	OpusEncoderNode();
	~OpusEncoderNode();
    void _ready();

	PackedByteArray encode(const PackedByteArray rawPcm);
};

}

#endif // OPUS_GDEXTENSION_OPUSENCODERNODE_H
