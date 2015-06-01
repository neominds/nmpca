package ronanki.swing;


import javax.swing.*;

import java.awt.event.*;
import java.awt.Desktop;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
/**
 *
 * @author K.Tata Rao 
 */
@SuppressWarnings("unused")

public class Main {

    /**
     * @param args the command line argumentsSystem.out.println("match"+mat.group());System.out.println("match"+mat.group());
     */
    public static void main(String[] args) {
        // TODO code application logic here
    	
        try {
            // Set System L&FPlease Select Your Device From ComboBox
	    UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
    	}
        catch (ClassNotFoundException e) {
       		// handle exception
    	}
    	catch (InstantiationException e) {
       		// handle exception
    	}
    	catch (IllegalAccessException e) {
    		
    		
       		// handle exception
    	}
    	catch (UnsupportedLookAndFeelException e) {
       		// handle exception
    	}
                java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                try {
                	
					final PcahostsimUI pcah = new PcahostsimUI();
					pcah.setVisible(true);	
					
				} catch (IOException e) {
					
					e.printStackTrace();
				}
            }
        });
                
    
    }	

}
