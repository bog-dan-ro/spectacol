<?php // -*- Mode: perl -*-

/* fuse.inc: shared routines for the Fuse and libspectrum webpages
   Copyright (c) 1999-2007 Darren Salt, Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

  $fuse_title = NULL;
  $fuse_menu_header = NULL;
  $fuse_sections = array ();
  $fuse_related = array ();
  $first_menu = true;


  function fuse_title ($title, $imgs = false, $expire = 604800)
  {
    global $fuse_title, $lastmod, $canonical_url;
    if ($fuse_title != NULL)
      return;

    header ('Content-Type: text/html; charset=utf-8');
    header ('ETag: '.
	    sprintf ('%x-%x-%x', fileinode (__FILE__), ob_get_length (),
		     $stamp),
	    true);

    // Document is valid for $expire seconds from now (default null)
    if ($expire != NULL)
      header ('Expires: '.gmdate ('D, d M Y H:i:s', time() + $expire).' +0000');

    $fuse_title = $title;
    print <<<END_HEADER
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
 <title>$title</title>
 <link rel="stylesheet" title="Fancy theme" type="text/css" href="spectrum.css" />
</head>

END_HEADER;

    print $imgs ? "<body onload=\"page_loaded(this)\">\n" : "<body>\n";

    print <<<END_HEADER
 <script type="text/javascript"><!--
   var ie = navigator.appName == 'Microsoft Internet Explorer';
   if (ie) document.write ('<table style="width: 100%; border: none; margin: 0px; padding: 0px;"><tr><td class="header">');

   function showmenu (button)
   {
     var menu = button.parentNode;
     while (menu && (menu.nodeType != 1 || (menu.tagName != 'SPAN' && menu.tagName != 'span')))
       menu = menu.nextSibling;

     var i = (button.src.slice (-9) == '/open.png')
	     ? 'closed' : (button.src.slice (-11) == '/closed.png')
			  ? 'open' : null;

     if (menu != null && i != null)
     {
       button.src = 'images/' + i + '.png';
       button.alt = (i == 'closed') ? '+' : (i == 'open') ? '-' : '&middot;';
       menu.className = 'menuitems' + i;
     }
     return false;
   }

END_HEADER;

    if ($imgs)
      print <<<END_IMGSCRIPT
   var magnified_image = null;
   var orig_width = null;
   var orig_height = null;

   function page_loaded (body)
   {
     body.onresize = 'page_resized ()';
     var d = document.images;
     var i;
     for (i = 0; i < d.length; ++i)
       if (d[i].className == 'shot2' || d[i].className == 'shot4')
       {
	 var s = 0 + d[i].className.slice (4);
	 setsize (d[i], d[i].width / s, d[i].height / s);
	 d[i].style.visibility = 'visible';
       }
   }

   function page_resized ()
   {
     var d = document.images;
     var i;
     for (i = 0; i < d.length; ++i)
       if (d[i].className == 'shot2' || d[i].className == 'shot4')
         d[i].style.right = '32px'; // keep in sync with stylesheet!
   }

   function magnify (image, w, h)
   {
     if (magnified_image == image)
       return;
     if (magnified_image != null)
       reduce (magnified_image);
     magnified_image = image;
     orig_width = image.style.width;
     orig_height = image.style.height;
     setsize (image, w, h);
     image.style.zIndex = 2;
     image.onmouseout = function (ev) { reduce (this); };
     eval ("image.onclick = function (ev) {\
	      if (ie) ev = window.event;\
	      if (ev.button < 2 && (typeof (ev.modifiers) == 'undefined' ? !ev.ctrlKey : !(ev.modifiers & Event.CONTROL_MASK)))\
	      {\
		if (magnified_image == null) magnify (this, w, h);\
		else reduce (magnified_image);\
		return false;\
	      } else return true;\
	    };");
   }

   function reduce (image)
   {
     if (magnified_image == null || magnified_image != image)
       return;
     image.onmouseout = null;
     image.style.width = orig_width;
     image.style.height = orig_height;
     image.style.zIndex = 1;
     magnified_image = null;
   }

   function setsize (image, w, h)
   {
     image.style.width = w + 'px';
     image.style.height = h + 'px';
   }

END_IMGSCRIPT;

    print <<<END_HEADER
  // --></script>

 <div id="header" class="header"><div class="header-div">
  <span class="menuhover">
   <a href="http://www.shadowmagic.org.uk/spectrum/"><img src="images/grey.png" width="12" height="12" alt="&middot;" />Sinclair Spectrum</a>
  </span>
  <div class="logo"><img src="logo/fuse.jpeg" alt="Fuse" /></div>
  <span class="x">&bull;</span>

END_HEADER;
  }


  function fuse_menu_heading ($header)
  {
    global $fuse_menu_header;
    if ($fuse_menu_header == NULL)
      $fuse_menu_header = $header;
  }


  function fuse_section ($item, $title, $body)
  {
    global $fuse_sections;
    $fuse_sections[] = array (($item == NULL ? $title : $item), $title, $body);
  }


  function fuse_related ($item, $href)
  {
    global $fuse_related;
    $fuse_related[] = array ($item, $href);
  }


  function fuse_footer ()
  {
    global $fuse_menu_header, $fuse_title, $fuse_sections, $fuse_related;
    output_menu ($fuse_menu_header, '#-top-', $fuse_sections);
    if ($fuse_related != NULL)
      output_menu ($fuse_related[0][0], $fuse_related[0][1], array_slice ($fuse_related, 1));
    output_menu ('SF project page', 'http://sourceforge.net/projects/fuse-emulator/', array (array ('Files', 'http://sourceforge.net/projects/fuse-emulator/files/'), array ('SVN', 'http://sourceforge.net/p/fuse-emulator/code/')));

    print <<<END_MENU
 </div></div>

 <script type="text/javascript"><!--
   if (ie) document.write ('</td><td class="body">');
 // --></script>

 <div class="body"><div class="body-div">

  <a name="-top-"></a>
  <h1>$fuse_title</h1>


END_MENU;

    foreach ($fuse_sections as $section)
    {
      $href = preg_replace ('/[^[:alnum:]]+/', '_', $section[0]);
      $href = preg_replace ('/^_+|_+$/', '', $href);
      print "  <a name=\"$href\"></a>\n  <h2>$section[1]</h2>\n  $section[2]\n\n";
    }

    $lastmod = stat (getenv ('SCRIPT_FILENAME'));
    $lastmod_inc = stat (__FILE__);
    if ($lastmod_inc[10] > $lastmod[10])
      $lastmod = $lastmod_inc;
    $lastmod = gmdate ('j F Y H:i:s', $lastmod[10]);

    $canonical_url = 'http://fuse-emulator.sourceforge.net'.getenv ('REQUEST_URI');

    print <<<END_DOCUMENT

  <hr />

  <p class="footer">Any comments, etc to <a href="mailto:philip-fuse@shadowmagic.org.uk">Philip Kendall</a>. This page was last updated on $lastmod GMT.</p>
  <p class="footer">URL for this page: <a href="$canonical_url">$canonical_url</a>; the <a href="fuse-website.tar.gz">full source</a> is available.</p>

  <hr />

  <table class="buttons">
   <tr>
    <td rowspan="2"><a href="http://sourceforge.net/"><img src="http://sourceforge.net/sflogo.php?group_id=91293&amp;type=1" width="88" height="31" alt="SourceForge.net Logo" /></a></td>
    <td rowspan="2"><a href="http://burnallgifs.org/"><img src="images/smburn.png" alt="Burn all GIFs!" height="62" width="101" /></a></td>
    <td><a href="http://validator.w3.org/check/referer"><img src="images/valid-xhtml10.png" alt="Valid XHTML 1.0!" height="31" width="88" /></a></td>
   </tr>
   <tr>
    <td><a href="http://jigsaw.w3.org/css-validator/check/referer"><img width="88" height="31" src="images/valid-css.png" alt="Valid CSS!" /></a></td>
   </tr>
  </table>

 </div></div>

 <script type="text/javascript"><!--
   if (ie) document.write ('</tr></table>');
 // --></script>

</body>
</html>

END_DOCUMENT;
  }


  function mk_image ($url, $width, $height, $scale = 2)
  {
    global $image_caption_done;
    $h = $height / $scale;
    $w = $width / $scale;
    return '<a href="'.$url.'" class="image" style="width: '.$w.'px; height: '.$h.'px;"><img class="shot'.$scale.'" src="'.$url.'" alt="[screenshot]" width="'.$width.'" height="'.$height.'" onmouseover="magnify(this,'.$width.','.$height.')" /></a>';
  }


  function output_menu ($header, $href, $menu)
  {
    global $first_menu;
    $sub = $first_menu ? 'open' : 'closed';
    $alt = $first_menu ? '+' : '-';
    if (! $first_menu)
      print "  <span class=\"x\">&middot;</span>\n";
    $first_menu = false;

    if (count ($menu) > 0)
    {
      print <<<END_MENU_HEADER
  <span class="menuhover">
   <a href="$href">
    <img src="images/$sub.png" width="12" height="12" onclick="return showmenu (this)" alt="$alt" title="Click to open or close" /><big><strong>$header</strong></big>
   </a>
   <span class="menuitems$sub">

END_MENU_HEADER;
      foreach ($menu as $section)
      {
	if (preg_match ('/^http/', $section[1]))
	  $href = $section[1];
	else
        {
	  $href = preg_replace ('/[^[:alnum:]]+/', '_', $section[0]);
	  $href = preg_replace ('/^_+|_+$/', '', $href);
	  $href = "#$href";
        }
	print "    <span class=\"x\">&bull;</span> <a href=\"$href\">$section[0]</a>\n";
      }
      print "   </span>\n  </span>\n";
    }
    else
      print <<<END_MENU_HEADER
  <span class="menuhover"><a href="$href">
   <img src="images/grey.png" width="12" height="12" alt="&bull;" onclick="return false" /><big><strong>$header</strong></big>
  </a></span>

END_MENU_HEADER;
  }

?>
