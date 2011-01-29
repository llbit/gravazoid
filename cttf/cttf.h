/**
 * Copyright (c) 2002-2011 Jesper Öqvist <jesper@llbit.se>
 *
 * This file is part of cTTF.
 *
 * cTTF is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cTTF is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with cTTF.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * see cttf.c for version history
 */

#ifndef CTTF_CTTF_H
#define CTTF_CTTF_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "vector.h"
#include "shape.h"

#if 1
#define SWAP_ENDIAN_WORD(a) \
	((((0xFF00 & a) >> 8) | ((0x00FF & a) << 8)))
#define SWAP_ENDIAN_DWORD(a) \
	(((0xFF000000 & a) >> 24) | ((0x00FF0000 & a) >> 8) | \
	 ((0x0000FF00 & a) << 8) | ((0x000000FF & a) << 24))
#else
#define SWAP_ENDIAN_WORD(a) (a)
#define SWAP_ENDIAN_DWORD(a) (a)
#endif


// forward declarations
typedef struct ttf_gd_list	ttf_gd_list_t;
typedef struct ttf		ttf_t;
typedef struct ttf_glyph_header	ttf_glyph_header_t;
typedef struct ttf_glyph_data	ttf_glyph_data_t;
typedef struct ttf_table_header	ttf_table_header_t;

// begin API
ttf_t* ttf_new();
void ttf_free(ttf_t** obj);
ttf_t* ttf_load(FILE* file);
void ttf_set_ls_aw(
		ttf_t*			ttfobj,
		ttf_glyph_header_t*	gh,
		ttf_glyph_data_t*	gd,
		uint16_t		i);
void ttf_read_glyph(ttf_t*		ttfobj,
		FILE*			file,
		ttf_glyph_header_t*	gh,
		ttf_glyph_data_t*	gd,
		ttf_table_header_t*	glyf,
		uint32_t*		indextolocation,
		uint32_t		i);
void ttf_gd_list_add(
		ttf_gd_list_t**		list,
		ttf_glyph_data_t*	data);
void ttf_free_gd_list(ttf_gd_list_t**	obj);
void ttf_interpolate(
		ttf_t*			ttfobj,
		uint16_t		chr,
		vector_t**		points,
		uint16_t**		endpoints,
		float			scale);

// export a TTF character to a vector list
void ttf_export_chr_shape(
		ttf_t*			ttfobj,
		uint16_t		chr,
		shape_t*		shape);

struct ttf_gd_list {
	ttf_gd_list_t*		succ;
	ttf_glyph_data_t*	data;
};

typedef enum ttf_markings {
	TTFavailable,
	TTFunavailable,
	TTFexcluded
} ttf_markings_t;

struct ttf_table_header
{
	uint32_t	tag;
	uint32_t	checksum;
	uint32_t	offset;
	uint32_t	length;
};

typedef struct ttf_font_header
{
	uint32_t	version;
	uint32_t	font_revision;
	uint32_t	checksumAdjust;
	uint32_t	magic;
	uint16_t	flags;
	uint16_t	upem;
	uint8_t		created[8];
	uint8_t		modified[8];
	int16_t		xmin;
	int16_t		ymin;
	int16_t		xmax;
	int16_t		ymax;
	uint16_t	mac_style;
	uint16_t	lowest_rec_ppm;
	int16_t		direction_hint;
	int16_t		index_to_loc_format;
	int16_t		glyph_data_format;

} ttf_font_header_t;

// unused?
typedef struct ttf_vertex
{
	ttf_markings_t	mark;
	uint16_t	index;
	uint16_t	le;
	uint16_t	re;
} ttf_vertex_t;

typedef struct ttf_table_directory
{
	uint32_t	sfnt_version;
	uint16_t	num_tables;
	uint16_t	search_range;
	uint16_t	entry_selector;
	uint16_t	range_shift;
} ttf_tbl_directory_t;

typedef struct ttf_encoding_table_info
{
	uint16_t	table_version;
	uint16_t	num_encoding_tables;
} ttf_enctbl_info_t;

typedef struct ttf_encoding_table_header
{
	uint16_t	platform_id;
	uint16_t	encoding_id;
	uint32_t	offset;
} ttf_enctbl_header_t;

typedef struct ttf_map_format0_header
{
	uint16_t	format;
	uint16_t	length;
	uint16_t	version;
	uint8_t		glyph_id_array[256];
} ttf_mapfmt0_header_t;

typedef struct ttf_map_format2_header
{
	// TODO
} ttf_mapfmt2_header_t;

typedef struct ttf_map_format4_header
{
	uint16_t	format;
	uint16_t	length;
	uint16_t	version;
	uint16_t	seg_count_2;
	uint16_t	search_range;
	uint16_t	entry_selector;
	uint16_t	range_shift;
	uint16_t*	end_count;
	uint16_t	reserved_pad;
	uint16_t*	start_count;
	int16_t*	id_delta;
	uint16_t*	id_range_offset;
	uint16_t*	glyph_id_array;
} ttf_mapfmt4_header_t;

typedef struct ttf_map_format6_header
{
	// TODO
} ttf_mapfmt6_header_t;

struct ttf_glyph_header
{
	int16_t	number_of_contours;
	int16_t	xmin;
	int16_t	ymin;
	int16_t	xmax;
	int16_t	ymax;
};

typedef struct ttf_simple_glyph_info
{
	uint16_t*	endpoints_contour;
	uint16_t	instruction_length;
	uint8_t*	instructions;
	uint8_t*	flags;
} ttf_simpglyphinfo_t;

// unused?
typedef struct ttf_horizontal_table_header
{
	uint32_t	version;
	int16_t		ascender;
	int16_t		descender;
	int16_t		linegap;
	uint16_t	advanceWidthMax;
	int16_t		minLeftBearing;
	int16_t		minRightBearing;
	int16_t		xMaxExtent;
	int16_t		caretSlopeRise;
	int16_t		caretSlopeRun;
	int16_t		reserved01;
	int16_t		reserved02;
	int16_t		reserved03;
	int16_t		reserved04;
	int16_t		reserved05;
	int16_t		metricDataFormat;
	uint16_t	numberOfHMetrics;
} ttf_horiz_tbl_header_t;

typedef struct ttf_maximum_profile_table_header
{
	uint32_t	version;
	uint16_t	numGlyphs;
	uint16_t	maxPoints;
	uint16_t	maxContours;
	uint16_t	maxCompositePoints;
	uint16_t	maxCompositeContours;
	uint16_t	maxZones;
	uint16_t	maxTwilightPoints;
	uint16_t	maxStorage;
	uint16_t	maxFunctionDefs;
	uint16_t	maxInstructionDefs;
	uint16_t	maxStackElements;
	uint16_t	maxSizeOfInstructions;
	uint16_t	maxComponentElements;
	uint16_t	maxComponentDepth;
} ttf_max_profile_tbl_header_t;

typedef struct ttf_horizontal_header
{
	uint32_t	version;
	int16_t		ascender;
	int16_t		descender;
	int16_t		linegap;
	uint16_t	advanceWidthMax;
	int16_t		minLeftSideBearing;
	int16_t		minRightSideBearing;
	int16_t		xMaxExtent;
	int16_t		caretSlopeRise;
	int16_t		caretSlopeRun;
	int16_t		reserved01;
	int16_t		reserved02;
	int16_t		reserved03;
	int16_t		reserved04;
	int16_t		reserved05;
	int16_t		metricDataFormat;
	uint16_t	num_h_metrics;
} ttf_horizontal_header_t;

typedef struct ttf_long_hmetrics
{
	uint16_t	aw;
	int16_t		lsb;
} ttf_lhmetrics_t;

struct ttf_glyph_data
{
	int16_t*	px;
	int16_t*	py;
	uint16_t*	endpoints;
	bool*		state;
	uint16_t	npoints;
	uint8_t		ncontours;
	uint16_t	aw;
	int16_t		lsb;
	uint16_t	maxwidth;
};

struct ttf {
	uint32_t*		glyph_table;
	ttf_glyph_data_t*	glyph_data;
	uint16_t		nglyphs;
	uint16_t		upem;
	uint16_t		ppem;
	uint16_t		resolution;
	uint8_t			interpolation_level;
	bool			zerobase;
	bool			zerolsb;
	uint32_t		nhmtx;
	ttf_lhmetrics_t*	plhmtx;
	int16_t*		plsb;
	int16_t			xmin;
	int16_t			ymin;
	int16_t			xmax;
	int16_t			ymax;
};


/*class cTTF
{
public:
	cTTF();
	~cTTF();

	void Load(const jsh::string& filename);

	void Convert(cMesh& mesh, uint16_t c) const;
	
	precision PointToPixelScale() const {return (ppem * resolution) / (72 * (precision)upem);}
	uint16_t PixelSize(uint16_t point_size) const {return uint16_t(point_size * PointToPixelScale() + 0.5f);}
	uint16_t RasterWidth() const {return PixelSize(xmax - xmin);}
	uint16_t RasterHeight() const {return PixelSize(ymax - ymin);}
	t_real GetWidth(uint16_t c) {return (t_real)(glyph_data[glyph_table[c]].aw) / upem;}

	void Resolution(uint16_t res) {resolution = res;}
	void EMPointSize(uint16_t point_size) {ppem = point_size;}

	void InterpolationLevel(uint8_t level) {interpolation_level = level;}
	uint8_t InterpolationLevel() const {return interpolation_level;}

private:
	void Reset();

	void ReadGlyph(cFile&, ttf_glyph_header_t&, ttf_glyph_data_t&, ttf_table_header_t&, uint32_t*, uint32_t);
	uint16_t Interpolate(uint16_t, jsh::vector2<t_real>*, jsh::vector2<t_real>*, uint16_t&, uint16_t) const;
	void ttf_set_ls_aw(ttf_glyph_header_t&, ttf_glyph_data_t&, uint16_t);
	void Interpolate(uint16_t c, jsh::vector2<t_real>*& points, uint16_t*& endpoints, uint16_t& npoints, precision scale) const;
};*/

#endif

