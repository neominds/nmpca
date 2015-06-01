package ronanki.swing;
import java.io.File;
import java.io.IOException;
import java.io.Serializable;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

@SuppressWarnings("serial")
public class ConfigBean implements Serializable {

	/**
	 * @param args
	 */
	static Element eElement;
	private String address,path;
	private int port;
	String[] server = new String[3];
	public ConfigBean(){
		try {
			
			File fXmlFile = new File("conf.xml");
				
			DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
			DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
			Document doc = dBuilder.parse(fXmlFile);
			doc.getDocumentElement().normalize();
 
			NodeList nList = doc.getElementsByTagName("Attributes");
 
			for (int temp = 0; temp < nList.getLength(); temp++) {
 
				Node nNode = nList.item(temp);
 
				if (nNode.getNodeType() == Node.ELEMENT_NODE) {
 
					 eElement = (Element) nNode;							
					server[temp] = eElement.getElementsByTagName("value").item(0).getTextContent();
				}
			}
			
			
			setServeraddress(server[0]);
			setServerport(Integer.parseInt(server[1]));
			setLogfilepath(server[2]);
		} catch (DOMException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (ParserConfigurationException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (SAXException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			//e.printStackTrace();
			System.err.println("Could not locate the conf file,Please verify conf.xml");
			System.exit(1);
		}
		catch(ArrayIndexOutOfBoundsException ae){
			System.err.println("Please do not add more nodes, conf.xml should contain only three nodes");
			System.exit(1);
		}
		catch(NumberFormatException ne){
			System.err.println("serverPort or serverAddress node missed in conf.xml");
			System.exit(1);
		}
		
	}
	
	public void setServeraddress(String addr){
		this.address = addr;
	}
	public String getServeraddess(){
		return address;
	}
	public void setServerport(int port){
		this.port = port;
	}
	public int getServerport(){
		return port;
	}
	public void setLogfilepath(String path){
		this.path = path;
	}
	public String getLogfilepath(){
		return path;
	}
}