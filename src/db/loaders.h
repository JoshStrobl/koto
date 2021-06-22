/* loaders.h
 *
 * Copyright 2021 Joshua Strobl
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

int process_artists(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_artist_paths(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_albums(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_playlists(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_playlists_tracks(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_tracks(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_track_paths(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

void read_from_db();