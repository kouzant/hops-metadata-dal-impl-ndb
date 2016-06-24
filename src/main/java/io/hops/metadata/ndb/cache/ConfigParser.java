package io.hops.metadata.ndb.cache;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by antonis on 6/23/16.
 */
public class ConfigParser {
    private final Log LOG = LogFactory.getLog(ConfigParser.class);

    private final File config;
    private final DocumentBuilder builder;

    public ConfigParser(String filename) throws IOException, ParserConfigurationException {
        URL res = ConfigParser.class.getClassLoader().getResource(filename);
        if (res == null) {
            throw new IOException("Could not find cache config resource");
        }
        config = new File(res.getFile());
        builder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
    }

    public CacheConfig parse() {
        try {
            Document doc = builder.parse(config);
            CacheConfigBuilder configBuilder = new CacheConfigBuilder();

            int threadLimit = Integer.parseInt(doc.getElementsByTagName("threadLimit").item(0)
                    .getAttributes().item(0).getTextContent());

            int sessions = Integer.parseInt(doc.getElementsByTagName("sessions").item(0)
                    .getAttributes().item(0).getTextContent());

            int sessionsPerThread = Integer.parseInt(doc.getElementsByTagName("sessionsPerThread").item(0)
                    .getAttributes().item(0).getTextContent());

            int sessionsInterval = Integer.parseInt(doc.getElementsByTagName("sessionsInterval").item(0)
                    .getAttributes().item(0).getTextContent());

            List<CachedDTO> cachedDTOs = new ArrayList<CachedDTO>();
            NodeList dtos = doc.getElementsByTagName("dto");

            for (int i = 0; i < dtos.getLength(); ++i) {
                Element elem = (Element) dtos.item(i);
                Class className = Class.forName(
                        elem.getAttribute("class"));
                Class[] intfs = className.getDeclaredClasses();
                String dtoName = elem.getElementsByTagName("name").item(0).getTextContent();

                Class dtoIntf = null;
                for (int j = 0; j < intfs.length; ++j) {
                    if (intfs[j].getName().endsWith(dtoName)) {
                        dtoIntf = intfs[j];
                        break;
                    }
                }
                int initSize = Integer.parseInt(
                        elem.getElementsByTagName("initialSize").item(0).getTextContent());
                int maxSize = Integer.parseInt(
                        elem.getElementsByTagName("maxSize").item(0).getTextContent());
                int step = Integer.parseInt(
                        elem.getElementsByTagName("step").item(0).getTextContent());

                cachedDTOs.add(new CachedDTO(dtoIntf, initSize, maxSize, step));
            }


            configBuilder.addThreadLimit(threadLimit).addCachedDTOs(cachedDTOs)
                    .addSessions(sessions).addSessionsPerThread(sessionsPerThread)
                    .addSessionsInterval(sessionsInterval);
            return configBuilder.buildConfig();
        } catch (IOException ex) {
            LOG.error("Error while parsing DTO cache configuration file", ex);
            ex.printStackTrace();
        } catch (SAXException ex) {
            LOG.error("Error while parsing DTO cache configuration file", ex);
            ex.printStackTrace();
        } catch (ClassNotFoundException ex) {
            LOG.error("Error while parsing DTO cache configuration file", ex);
            ex.printStackTrace();
        }

        return null;
    }

    private class CacheConfigBuilder {
        private int threadLimit = 2;
        private int sessions = 20;
        private int sessionsPerThread = 5;
        private int sessionsInterval = 10;
        private List<CachedDTO> cachedDTOs = new ArrayList<CachedDTO>();

        public CacheConfigBuilder() {
        }

        public CacheConfigBuilder addThreadLimit(int threadLimit) {
            this.threadLimit = threadLimit;
            return this;
        }

        public CacheConfigBuilder addSessionsPerThread(int sessionsPerThread) {
            this.sessionsPerThread = sessionsPerThread;
            return this;
        }

        public CacheConfigBuilder addSessions(int sessions) {
            this.sessions = sessions;
            return this;
        }

        public CacheConfigBuilder addSessionsInterval(int sessionsInterval) {
            this.sessionsInterval = sessionsInterval;
            return this;
        }

        public CacheConfigBuilder addCachedDTOs(List<CachedDTO> cachedDTOs) {
            this.cachedDTOs = cachedDTOs;
            return this;
        }

        public CacheConfig buildConfig() {
            return new CacheConfig(threadLimit, sessions, sessionsPerThread, sessionsInterval, cachedDTOs);
        }
    }

    public class CachedDTO {
        private final Class dtoClass;
        private final int initSize;
        private final int maxSize;
        private final int step;

        public CachedDTO(Class dtoClass, int initSize, int maxSize, int step) {
            this.dtoClass = dtoClass;
            this.initSize = initSize;
            this.maxSize = maxSize;
            this.step = step;
        }

        public Class getDtoClass() {
            return dtoClass;
        }

        public int getInitSize() {
            return initSize;
        }

        public int getMaxSize() {
            return maxSize;
        }

        public int getStep() {
            return step;
        }

        @Override
        public String toString() {
            return "[" + dtoClass.getName() + "] " + initSize + " - " + maxSize + " - " + step;
        }
    }

    public class CacheConfig {
        private final int threadLimit;
        private final int sessions;
        private final int sessionsPerThread;
        private final int sessionsInterval;
        private final List<CachedDTO> cachedDTOs;

        public CacheConfig(int threadLimit, int sessions, int sessionsPerThread,
                int sessionsInterval, List<CachedDTO> cachedDTOs) {
            this.threadLimit = threadLimit;
            this.sessions = sessions;
            this.sessionsPerThread = sessionsPerThread;
            this.sessionsInterval = sessionsInterval;
            this.cachedDTOs = cachedDTOs;
        }

        public int getThreadLimit() {
            return threadLimit;
        }

        public int getSessions() {
            return sessions;
        }

        public int getSessionsPerThread() {
            return sessionsPerThread;
        }

        public int getSessionsInterval() {
            return sessionsInterval;
        }

        public List<CachedDTO> getCachedDTOs() {
            return cachedDTOs;
        }
    }
}
