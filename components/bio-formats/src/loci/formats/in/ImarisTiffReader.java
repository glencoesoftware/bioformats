//
// ImarisTiffReader.java
//

/*
OME Bio-Formats package for reading and converting biological file formats.
Copyright (C) 2005-@year@ UW-Madison LOCI and Glencoe Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

package loci.formats.in;

import java.io.IOException;
import java.util.StringTokenizer;
import java.util.Vector;

import loci.formats.FormatException;
import loci.formats.FormatTools;
import loci.formats.MetadataTools;
import loci.formats.meta.MetadataStore;
import loci.formats.tiff.IFD;
import loci.formats.tiff.IFDList;

import ome.xml.model.primitives.PositiveInteger;

/**
 * ImarisTiffReader is the file format reader for
 * Bitplane Imaris 3 files (TIFF variant).
 *
 * <dl><dt><b>Source code:</b></dt>
 * <dd><a href="https://skyking.microscopy.wisc.edu/trac/java/browser/trunk/components/bio-formats/src/loci/formats/in/ImarisTiffReader.java">Trac</a>,
 * <a href="https://skyking.microscopy.wisc.edu/svn/java/trunk/components/bio-formats/src/loci/formats/in/ImarisTiffReader.java">SVN</a></dd></dl>
 *
 * @author Melissa Linkert linkert at wisc.edu
 */
public class ImarisTiffReader extends BaseTiffReader {

  // -- Constructor --

  /** Constructs a new Imaris TIFF reader. */
  public ImarisTiffReader() {
    super("Bitplane Imaris 3 (TIFF)", "ims");
    suffixSufficient = false;
    suffixNecessary = true;
    domains = new String[] {FormatTools.GRAPHICS_DOMAIN};
  }

  // -- Internal FormatReader API methods --

  /* @see loci.formats.FormatReader#initFile(String) */
  protected void initFile(String id) throws FormatException, IOException {
    super.initFile(id);

    // hack up the IFDs
    //
    // Imaris TIFFs store a thumbnail in the first IFD; each of the remaining
    // IFDs defines a stack of tiled planes.
    // MinimalTiffReader.initFile(String) removes thumbnail IFDs.

    LOGGER.info("Verifying IFD sanity");

    IFDList tmp = new IFDList();

    for (IFD ifd : ifds) {
      long[] byteCounts = ifd.getStripByteCounts();
      long[] offsets = ifd.getStripOffsets();

      for (int i=0; i<byteCounts.length; i++) {
        IFD t = new IFD(ifd);
        t.putIFDValue(IFD.TILE_BYTE_COUNTS, byteCounts[i]);
        t.putIFDValue(IFD.TILE_OFFSETS, offsets[i]);
        tmp.add(t);
      }
    }

    String comment = ifds.get(0).getComment();

    LOGGER.info("Populating metadata");

    core[0].sizeC = ifds.size();
    core[0].sizeZ = tmp.size() / getSizeC();
    core[0].sizeT = 1;
    core[0].sizeX = (int) ifds.get(0).getImageWidth();
    core[0].sizeY = (int) ifds.get(0).getImageLength();

    ifds = tmp;
    core[0].imageCount = getSizeC() * getSizeZ();
    core[0].dimensionOrder = "XYZCT";
    core[0].interleaved = false;
    core[0].rgb = getImageCount() != getSizeZ() * getSizeC() * getSizeT();
    core[0].pixelType = ifds.get(0).getPixelType();

    LOGGER.info("Parsing comment");

    // likely an INI-style comment, although we can't be sure

    MetadataStore store = makeFilterMetadata();
    MetadataTools.populatePixels(store, this);

    if (getMetadataOptions().getMetadataLevel() == MetadataLevel.ALL) {
      String description = null, creationDate = null;
      Vector<Integer> emWave = new Vector<Integer>();
      Vector<Integer> exWave = new Vector<Integer>();
      Vector<String> channelNames = new Vector<String>();

      if (comment != null && comment.startsWith("[")) {
        // parse key/value pairs
        StringTokenizer st = new StringTokenizer(comment, "\n");
        while (st.hasMoreTokens()) {
          String line = st.nextToken();
          int equals = line.indexOf("=");
          if (equals < 0) continue;
          String key = line.substring(0, equals).trim();
          String value = line.substring(equals + 1).trim();
          addGlobalMeta(key, value);

          if (key.equals("Description")) {
            description = value;
          }
          else if (key.equals("LSMEmissionWavelength") && !value.equals("0")) {
            emWave.add(new Integer(value));
          }
          else if (key.equals("LSMExcitationWavelength") && !value.equals("0"))
          {
            exWave.add(new Integer(value));
          }
          else if (key.equals("Name") && !currentId.endsWith(value)) {
            channelNames.add(value);
          }
          else if (key.equals("RecordingDate")) {
            value = value.replaceAll(" ", "T");
            creationDate = value.substring(0, value.indexOf("."));
          }
        }
        metadata.remove("Comment");
      }

      // populate Image data
      store.setImageDescription(description, 0);
      store.setImageAcquiredDate(creationDate, 0);

      // populate LogicalChannel data
      for (int i=0; i<emWave.size(); i++) {
        store.setChannelEmissionWavelength(
          new PositiveInteger(emWave.get(i)), 0, i);
        store.setChannelExcitationWavelength(
          new PositiveInteger(exWave.get(i)), 0, i);
        store.setChannelName(channelNames.get(i), 0, i);
      }
    }
  }

}
