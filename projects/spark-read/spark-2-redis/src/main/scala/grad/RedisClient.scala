package grad  
  
import java.io.{InputStream, BufferedInputStream, FileInputStream}  
import java.util.Properties  
  
import org.apache.commons.pool2.impl.GenericObjectPoolConfig  
import redis.clients.jedis.JedisPool  
import scala.collection.JavaConversions._  
  
object RedisClient extends Serializable {  
    
  val properties: Properties = new Properties  
  val in: InputStream = getClass.getResourceAsStream("/redis.properties")  
  properties.load(new BufferedInputStream(in))  
  val redisHost = properties.getProperty("redis.host")  
  val redisPort = properties.getProperty("redis.port")  

  val redisTimeout = 30000  
  val config = new GenericObjectPoolConfig()  
  config.setTestOnBorrow(true)  
  config.setMaxIdle(properties.getProperty("redis.maxidle").toInt)  
  config.setMinIdle(properties.getProperty("redis.minidle").toInt)  
  config.setMaxTotal(properties.getProperty("redis.maxtotal").toInt)  
    
  lazy val pool = new JedisPool(config, redisHost, redisPort.toInt, redisTimeout)  
  
  lazy val hook = new Thread {  
    override def run = {  
      println("Execute hook thread: " + this)  
      pool.destroy() 
    } 
  } 
  sys.addShutdownHook(hook.run)  
}  
