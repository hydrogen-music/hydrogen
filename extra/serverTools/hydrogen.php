<?php
	/*
	* Hydrogen Web Toolkit
	* Copyright(c) 2008 by Sebastian Moors [mauser@smoors.de]
	*
	* http://www.hydrogen-music.org
	*
	* This program is free software; you can redistribute it and/or modify
	* it under the terms of the GNU General Public License as published by
	* the Free Software Foundation; either version 2 of the License, or
	* (at your option) any later version.
	*
	* This program is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY, without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	* GNU General Public License for more details.
	*
	* You should have received a copy of the GNU General Public License
	* along with this program; if not, write to the Free Software
	* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	*
	*/


	/*
		Use this script if you want to share your hydrogen songs,patterns and
		drumkits with other people. It generates a xml-file which can be used
		with the hydrogen soundlibrary.

		Just place this script in one directory with all your drumkits, songs 
		and patterns and make it available via a webserver.

		Songs and patterns could be parsed in place. Since drumkits are archives,
		we cannot parse the files at runtime. Therefore, just the name and the url of the drumkit will be used. 
		If you want additional informations ( such as author / info ) take a look
		at the metaInfo.inc file (the file has to be in the same directory as this script).
		
	*/


	/*
		metaInfo.inc holds meta informations about the drumkits, namely:
		
		- url
		- author 
		- info
		- name

		If metaInfo.inc is not available, the script takes the filename as namely	and guesses the url
	*/

	if ( is_file("metaInfo.inc") ) {
		include("metaInfo.inc");
	}


	function getTag( $xml , $tag ){
		preg_match( "/". $tag . "(.*)</" , $xml , $found );
		if( ISSET( $found[1] ) ) {
			return $found[1];
		} 
	}


	//$url should be defined in metaInfo.inc
	if( ! ISSET( $url ) ){
		$url = $_SERVER [ 'SERVER_NAME' ] ;
		$dir = dirname ( $_SERVER['PHP_SELF'] ) ;
		$url = "http://$url$dir";
	}

	$author = "";
	$info = "";

	/* Start of xml-document */

	print "<?xml version='1.0' encoding='UTF-8'?>\n";
	print "<drumkit_list>\n";


	/* Start of song listing */
	

	$dir = "./";
	if ($dh = opendir( $dir )) {
        	while (( $file = readdir($dh) ) !== false) {
			$extension = array_pop( explode(".", $file) );
			if( $extension == "h2song" ) {
				print "\t<song>\n";
				$content = file( $dir.$file );
				$xml = join( " ",$content );
				print "\t\t<name>" . getTag( $xml , "<name>" ) . "</name>\n";
				print "\t\t<url>" . $url.$file ."</url>\n";
				print "\t\t<author>" . getTag( $xml , "<author>" ) . "</author>\n";
				print "\t\t<info>" . getTag( $xml , "<info>" ) . "</info>\n";
				print "\t</song>\n";
			}
        	}
	    	closedir( $dh );
    	}
	/* End of song listing */



	/* Start of pattern listing */
	$dir = "./";
	if ($dh = opendir( $dir )) {
        	while ( ( $file = readdir( $dh ) ) !== false ) {
			$extension = array_pop(explode( ".", $file ));
			if( $extension == "h2pattern" ) {
				$content = file( $dir.$file );
				$xml = join( " " , $content );
				print "\t<pattern>\n";
				print "\t\t<name>" . getTag( $xml , "<name>" ) . "</name>\n";
				print "\t\t<url>" . $url.$file ."</url>\n";
				print "\t</pattern>\n";
			}
        	}
	    	closedir( $dh );
    	}	
	/* End of pattern listing */


	/* Start of drumkit listing */
	$dir = "./";
	if ($dh = opendir( $dir ) ) {
        	while ( ( $file = readdir( $dh ) ) !== false) {
			$extension = array_pop( explode( ".", $file ) );
			if( $extension == "h2drumkit" ) {
				$content = file( $dir.$file );
				$xml = join( " " , $content );
				print "\t<drumkit>\n";
			
				//name: filename without extension
				$name = basename( $file,".h2drumkit" );

				If( ISSET( $drumkit_list[ $name ]) ){
					$author = $drumkit_list[ $name ][ "author" ];
					$info = $drumkit_list[ $name ][ "info" ];
				}

				print "\t\t<name> $name </name>\n";
				print "\t\t<url>" . $url.$file ."</url>\n";
				print "\t\t<author>$author</author>\n";
				print "\t\t<info>$info</info>\n";
				print "\t</drumkit>\n";
			}
        	}
	    	closedir( $dh );
    	}	
	/* End of pattern listing */


	print "</drumkit_list>\n";
		

?>
