import java.io.IOException;
import org.apache.pdfbox.preflight.parser.PreflightParser;
import org.apache.pdfbox.preflight.*;
import org.apache.pdfbox.preflight.exception.SyntaxValidationException;

public class PdfBoxValidator {

    public static void main(String[] args) {
        org.apache.pdfbox.preflight.ValidationResult result = null;

        if (args.length != 1) {
            System.err.println("Require exactly one filename as command line argument");
            System.exit(1);
        }

        try
        {
            PreflightParser parser = new PreflightParser(args[0]);

            /* Parse the PDF file with PreflightParser that inherits from the NonSequentialParser.
             * Some additional controls are present to check a set of PDF/A requirements.
             * (Stream length consistency, EOL after some Keyword...)
             */
            parser.parse();

            /* Once the syntax validation is done,
             * the parser can provide a PreflightDocument
             * (that inherits from PDDocument)
             * This document process the end of PDF/A validation.
             */
            PreflightDocument document = parser.getPreflightDocument();
            document.validate();

            // Get validation result
            result = document.getResult();
            document.close();

        }
        catch (SyntaxValidationException e)
        {
            /* the parse method can throw a SyntaxValidationException
             * if the PDF file can't be parsed.
             * In this case, the exception contains an instance of ValidationResult
             */
            result = e.getResult();
        }
        catch (IOException e)
        {
            System.err.println("IO error when opening " + args[0]);
            System.exit(1);
        }

        /// display validation result
        if (result.isValid())
        {
            System.out.println("The file '" + args[0] + "' is a valid PDF/A-1b file");
            System.exit(0);
        }
        else
        {
            System.out.println("The file '" + args[0] + "' is NOT PDF/A-1b valid, " + result.getErrorsList().size() + " error(s)");
            System.exit(0);
        }
    }

}
