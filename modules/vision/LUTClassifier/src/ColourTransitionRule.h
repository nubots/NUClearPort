/*
 * This file is part of LUTClassifier.
 *
 * LUTClassifier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LUTClassifier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LUTClassifier.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#ifndef COLOURTRANSITIONRULE_H
#define COLOURTRANSITIONRULE_H

#include "messages/vision/ClassifiedImage.h"
#include "LookUpTable.h"

modules {
	vision { 
		class ColourTransitionRule {
		public:
		    static ColourSegment nomatch;   //! @variable A static segment used to represent one that cannot be matched to any rule.

		    ColourTransitionRule();
		    /*!
		      Checks if the given segment pair matches this rule (forward and reverse).
		      @param before the preceeding segment.
		      @param middle the middle segment.
		      @param after the following segment.
		      @return Whether it is a match in either direction.
		      */
		    bool match(const ColourSegment& before, const ColourSegment& middle, const ColourSegment& after) const;
		    //! Returns the ID of the field object that this rule is for.
		    messages::vision::ClassifiedImage::COLOUR_CLASS getColourClass() const;		   

		private:
		    
		    messages::vision::ClassifiedImage::COLOUR_CLASS m_colour_class;     //! @variable The ID of the field object that this rule is for.

		    std::vector<messages::vision::ClassifiedImage::Colour>  m_before,   //! @variable The colour that the previous segment must be.
		                    	 m_middle,   //! @variable The colour that this segment must be
		                    	 m_after;    //! @variable The colour that the following segment must be.

		    unsigned int m_before_min,   //! @variable the minimum length of the previous segment for a match.
				         m_before_max,   //! @variable the maximum length of the previous segment for a match.
				         m_min,          //! @variable the minimum length of the segment for a match.
				         m_max,          //! @variable the maximum length of the segment for a match.
				         m_after_min,    //! @variable the minimum length of the following segment for a match.
				         m_after_max;    //! @variable the maximum length of the following segment for a match.
		};
	}
}
#endif