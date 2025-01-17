"""
USAGE:
{Contact point whole number} and {numerator} over {denominator} at {wheel location}
{Contact point whole number} at {wheel location}

If the delay after a sentence is too long, find and replace "large-v2" with "base.en" or "tiny.en".
"""

#graph = []
x = []
y = []
finished = False
plot_title = input("Graph title: ")

def clear_console():
	"""Clears the console."""

	# For Windows
	if os.name == 'nt':
		_ = os.system('cls')

	# For macOS and Linux
	else:
		_ = os.system('clear')

def plot_results():
	plt.title(plot_title)
	plt.xlabel("Wheel position")
	plt.ylabel("Contact point")
	plt.scatter(x, y) 
	plt.show()

def graph_text(text):
	global finished

	rcp_whole = 0
	numerator = 0
	denominator = 0
	wheel_location = 0

	print(text)
	text = text.replace(".", "")
	if "graph is finished" in text.lower():
		finished = True
		return
	any_errors = False
	split_words = text.split()
	if len(split_words) == 7:
		if split_words[0].isdigit():
			rcp_whole = split_words[0]
		else:
			any_errors = True
		if split_words[1].lower() != "and":
			any_errors = True
		if split_words[2].isdigit():
			numerator = split_words[2]
		else:
			any_errors = True
		if split_words[3].lower() != "over":
			any_errors = True
		if split_words[4].isdigit():
			denominator = split_words[4]
		else:
			any_errors = True
		if split_words[5].lower() != "at":
			any_errors = True
		if split_words[6].isdigit():
			wheel_location = split_words[6]
		else:
			any_errors = True
		if not any_errors:
			if int(wheel_location) in x:
				y[x.index(int(wheel_location))] = int(rcp_whole) + int(numerator) / int(denominator)
			else:
				x.append(int(wheel_location))
				y.append(int(rcp_whole) + int(numerator) / int(denominator))
			#graph.append((wheel_location, int(rcp_whole) + int(numerator) / int(denominator)))
		else:
			print("ERROR!")

	elif len(split_words) == 3:
		if split_words[0].isdigit():
			rcp_whole = split_words[0]
		else:
			any_errors = True
		if split_words[1].lower() != "at":
			any_errors = True
		if split_words[2].isdigit():
			wheel_location = split_words[2]
		else:
			any_errors = True
		if not any_errors:
			if int(wheel_location) in x:
				y[x.index(int(wheel_location))] = int(rcp_whole)
			else:
				x.append(int(wheel_location))
				y.append(int(rcp_whole))
			#graph.append((wheel_location, rcp_whole))
	else:
		print("ERROR!")

def process_text(text):
	graph_text(text)

if __name__ == '__main__':

	import matplotlib.pyplot as plt
	import os
	import sys
	from RealtimeSTT import AudioToTextRecorder


	if os.name == "nt" and (3, 8) <= sys.version_info < (3, 99):
		from torchaudio._extension.utils import _init_dll_path
		_init_dll_path()

	end_of_sentence_detection_pause = 0.45
	unknown_sentence_detection_pause = 0.7
	mid_sentence_detection_pause = 2.0

	# Recorder configuration
	recorder_config = {
		'spinner': False,
		'model': 'large-v2', # or large-v2 or deepdml/faster-whisper-large-v3-turbo-ct2 or ...
		'download_root': None, # default download root location. Ex. ~/.cache/huggingface/hub/ in Linux
		# 'input_device_index': 1,
		'realtime_model_type': 'large-v2', # or small.en or distil-small.en or ...
		'language': 'en',
		'silero_sensitivity': 0.05,
		'webrtc_sensitivity': 3,
		'post_speech_silence_duration': unknown_sentence_detection_pause,
		'min_length_of_recording': 1.1,        
		'min_gap_between_recordings': 0,                
		'enable_realtime_transcription': True,
		'realtime_processing_pause': 0.02,
		#'on_realtime_transcription_update': text_detected,
		#'on_realtime_transcription_stabilized': text_detected,
		'silero_deactivity_detection': True,
		'early_transcription_on_silence': 0,
		'beam_size': 5,
		'beam_size_realtime': 3,
		# 'batch_size': 0,
		# 'realtime_batch_size': 0,        
		'no_log_file': True,
		'initial_prompt': (
			"End incomplete sentences with ellipses.\n"
			"Examples:\n"
			"Complete: The sky is blue.\n"
			"Incomplete: When the sky...\n"
			"Complete: She walked home.\n"
			"Incomplete: Because he...\n"
		)
	}

print("Wait until it says 'speak now'")
recorder = AudioToTextRecorder()

clear_console()
while not finished:
	recorder.text(process_text)

recorder.shutdown()
plot_results()