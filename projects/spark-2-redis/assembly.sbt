import AssemblyKeys._ // put this at the top of the file

assemblySettings


test in assembly := {}


mainClass in assembly := Some( "hbase")


assemblyOption in packageDependency ~= { _.copy(appendContentHash = true) }

mergeStrategy in assembly := {
 case PathList("META-INF", xs @ _*) => MergeStrategy.discard
 case x => MergeStrategy.first
}

/*
mergeStrategy in assembly <<= (mergeStrategy in assembly) { (old) =>
{
  case PathList(ps @ _*) if ps.last endsWith "axiom.xml" => MergeStrategy.filterDistinctLines
  case PathList(ps @ _*) if ps.last endsWith "Log.class" => MergeStrategy.first
  case PathList(ps @ _*) if ps.last endsWith "LogConfigurationException.class" => MergeStrategy.first
  case PathList(ps @ _*) if ps.last endsWith "LogFactory.class" => MergeStrategy.first
  case PathList(ps @ _*) if ps.last endsWith "SimpleLog$1.class" => MergeStrategy.first
  case x => old(x)
}
}
*/

